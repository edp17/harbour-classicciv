#include <SDL.h>

#include <atomic>
#include <cerrno>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static std::atomic<bool> g_control_running{false};
static std::thread g_control_thread;
static std::string g_sock_path;

static bool parse_scancode_token(const std::string& token, SDL_Scancode& out)
{
    if (token.empty())
        return false;

    // Accept numeric scancode (what you currently send: KEYDOWN 1, etc)
    bool all_digits = true;
    for (char c : token) {
        if (c < '0' || c > '9') { all_digits = false; break; }
    }
    if (all_digits) {
        const int v = std::atoi(token.c_str());
        if (v <= 0 || v >= SDL_NUM_SCANCODES)
            return false;
        out = static_cast<SDL_Scancode>(v);
        return true;
    }

    // Accept human-friendly names: "1", "ENTER", "ESCAPE", "F1", ...
    // SDL recognises many names; case-insensitive in practice for common keys.
    SDL_Scancode sc = SDL_GetScancodeFromName(token.c_str());
    if (sc != SDL_SCANCODE_UNKNOWN) {
        out = sc;
        return true;
    }

    // If user passes a single character like "a" or "1" and SDL name lookup fails,
    // try mapping from keycode.
    if (token.size() == 1) {
        SDL_Keycode kc = SDL_GetKeyFromName(token.c_str());
        if (kc != SDLK_UNKNOWN) {
            sc = SDL_GetScancodeFromKey(kc);
            if (sc != SDL_SCANCODE_UNKNOWN) {
                out = sc;
                return true;
            }
        }
    }

    return false;
}

static int make_unix_server(const std::string &path)
{
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;

    // Ensure old socket is removed
    ::unlink(path.c_str());

    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;

    if (path.size() >= sizeof(addr.sun_path)) {
        ::close(fd);
        return -1;
    }
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }

    if (::listen(fd, 1) < 0) {
        ::close(fd);
        return -1;
    }

    return fd;
}

static inline void push_key(SDL_Scancode sc, bool down)
{
    SDL_Event ev {};
    ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    ev.key.repeat = 0;
    ev.key.keysym.scancode = sc;
    ev.key.keysym.sym = SDL_GetKeyFromScancode(sc);
    ev.key.keysym.mod = SDL_GetModState();
    SDL_PushEvent(&ev);
}

static inline void push_text(const std::string &utf8)
{
    SDL_Event ev {};
    ev.type = SDL_TEXTINPUT;
    std::memset(ev.text.text, 0, sizeof(ev.text.text));
    // SDL_TEXTINPUT supports up to 31 bytes + NUL
    std::strncpy(ev.text.text, utf8.c_str(), sizeof(ev.text.text) - 1);
    SDL_PushEvent(&ev);
}

static inline void push_mouse_button(uint8_t button, bool down)
{
    SDL_Event ev {};
    ev.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    ev.button.button = button;
    ev.button.state = down ? SDL_PRESSED : SDL_RELEASED;
    SDL_PushEvent(&ev);
}

static inline void push_mouse_move_rel(int dx, int dy)
{
    SDL_Event ev {};
    ev.type = SDL_MOUSEMOTION;
    ev.motion.xrel = dx;
    ev.motion.yrel = dy;
    SDL_PushEvent(&ev);
}

static SDL_Scancode parse_scancode(const std::string &name)
{
    // Accept e.g. "SDL_SCANCODE_1" or "1" or "RETURN"
    if (name.rfind("SDL_SCANCODE_", 0) == 0) {
        const auto s = name.substr(std::strlen("SDL_SCANCODE_"));
        return SDL_GetScancodeFromName(s.c_str());
    }
    // Let SDL parse common names like "1", "Return", "Escape", "F1"
    return SDL_GetScancodeFromName(name.c_str());
}

static void control_loop(std::string sock_path)
{
    g_sock_path = sock_path; // remember for Stop()
    const int server_fd = make_unix_server(sock_path);
    if (server_fd < 0)
        return;

    g_control_running.store(true);

    while (g_control_running.load()) {
        const int client_fd = ::accept(server_fd, nullptr, nullptr);
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            break;
        }

        // Simple line buffer
        std::string buf;
        buf.reserve(4096);

        char tmp[512];
        while (g_control_running.load()) {
            const ssize_t n = ::read(client_fd, tmp, sizeof(tmp));
            if (n <= 0)
                break;

            buf.append(tmp, tmp + n);

            // Process complete lines
            size_t pos = 0;
            while (true) {
                const size_t nl = buf.find('\n', pos);
                if (nl == std::string::npos) {
                    // Keep remainder
                    buf.erase(0, pos);
                    break;
                }

                std::string line = buf.substr(pos, nl - pos);
                pos = nl + 1;

                // Trim CR
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();

                if (line.empty())
                    continue;

                // Commands:
                // KEYDOWN <name>
                // KEYUP <name>
                // TEXT <utf8>
                // MOUSEMOVE <dx> <dy>    (relative)
                // MOUSEBTN <left|right> <down|up>
                auto starts = [&](const char *p){ return line.rfind(p, 0) == 0; };

                if (starts("KEYDOWN ")) {
                    const auto key = line.substr(8);
                    const SDL_Scancode sc = parse_scancode(key);
                    if (sc != SDL_SCANCODE_UNKNOWN)
                        push_key(sc, true);
                } else if (starts("KEYUP ")) {
                    const auto key = line.substr(6);
                    const SDL_Scancode sc = parse_scancode(key);
                    if (sc != SDL_SCANCODE_UNKNOWN)
                        push_key(sc, false);
                } else if (starts("KEY ")) {
                    const auto key = line.substr(4);
                    const SDL_Scancode sc = parse_scancode(key);
                    if (sc != SDL_SCANCODE_UNKNOWN) {
                        push_key(sc, true);   // key down
                        push_key(sc, false);  // key up
                    }
                } else if (starts("TEXT ")) {
                    const auto t = line.substr(5);
                    if (!t.empty())
                        push_text(t);
                } else if (starts("MOUSEMOVE ")) {
                    int dx = 0, dy = 0;
                    if (std::sscanf(line.c_str(), "MOUSEMOVE %d %d", &dx, &dy) == 2)
                        push_mouse_move_rel(dx, dy);
                } else if (starts("MOUSEBTN ")) {
                    char which[16] = {0};
                    char state[16] = {0};
                    if (std::sscanf(line.c_str(), "MOUSEBTN %15s %15s", which, state) == 2) {
                        const bool down = (std::strcmp(state, "down") == 0);
                        uint8_t btn = 0;
                        if (std::strcmp(which, "left") == 0)  btn = SDL_BUTTON_LEFT;
                        if (std::strcmp(which, "right") == 0) btn = SDL_BUTTON_RIGHT;
                        if (btn)
                            push_mouse_button(btn, down);
                    }
                }
            }
        }

        ::close(client_fd);
    }

    ::close(server_fd);
    ::unlink(sock_path.c_str());
}

extern "C" void CONTROL_Socket_Start()
{
    const char *p = std::getenv("DOSBOX_CONTROL_SOCKET");
    if (!p || !*p)
        return;

    // Don’t start twice
    if (g_control_thread.joinable())
        return;

    g_control_thread = std::thread(control_loop, std::string(p));
}

extern "C" void CONTROL_Socket_Stop()
{
    g_control_running.store(false);

    // Wake accept() by connecting once
    if (!g_sock_path.empty()) {
        int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0) {
            sockaddr_un addr {};
            addr.sun_family = AF_UNIX;
            // use strncpy/memcpy; keeping your memcpy preference:
            const auto bytes = std::min(g_sock_path.size(), sizeof(addr.sun_path) - 1);
            std::memcpy(addr.sun_path, g_sock_path.c_str(), bytes);
            addr.sun_path[bytes] = '\0';
            ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            ::close(fd);
        }
    }

    if (g_control_thread.joinable())
        g_control_thread.join();
}
