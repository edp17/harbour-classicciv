#include "DosboxRunner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <QTimer>

static bool send_to_control_socket(const QString &path, const QString &line)
{
    const QByteArray p = QFile::encodeName(path);
    const QByteArray msg = (line + "\n").toUtf8();

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        return false;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (p.size() >= int(sizeof(addr.sun_path))) {
        ::close(fd);
        return false;
    }

    const int n = p.size();
    if (n >= int(sizeof(addr.sun_path))) {
        ::close(fd);
        return false;
    }
    std::memcpy(addr.sun_path, p.constData(), size_t(n));
    addr.sun_path[n] = '\0';

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return false;
    }

    const char *data = msg.constData();
    ssize_t left = msg.size();
    while (left > 0) {
        const ssize_t n = ::write(fd, data, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            ::close(fd);
            return false;
        }
        data += n;
        left -= n;
    }

    ::close(fd);
    return true;
}

QString DosboxRunner::windowRes() const
{
    return m_windowRes;
}

QString DosboxRunner::controlSocketPath() const
{
    // Must match DOSBOX_CONTROL_SOCKET in launch()
    return QDir::temp().filePath(QStringLiteral("harbour-classicciv.sock"));
}

bool DosboxRunner::sendControlLine(const QString &line)
{
    return send_to_control_socket(controlSocketPath(), line);
}

bool DosboxRunner::sendKey(const QString &name)
{
    return sendControlLine(QStringLiteral("KEY %1").arg(name));
}

bool DosboxRunner::sendKeyDown(const QString &name)
{
    return sendControlLine(QStringLiteral("KEYDOWN %1").arg(name));
}

bool DosboxRunner::sendKeyUp(const QString &name)
{
    return sendControlLine(QStringLiteral("KEYUP %1").arg(name));
}

// Minimal ASCII text support: maps characters to KEY commands.
// Good enough for Civ city names, save names, etc.
static QString keyNameForChar(QChar c, bool *needsShift)
{
    *needsShift = false;

    if (c.isLetter()) {
        const QChar up = c.toUpper();
        if (c.isUpper()) *needsShift = true;
        return QString(up);
    }
    if (c.isDigit()) return QString(c);

    switch (c.unicode()) {
    case ' ': return "SPACE";
    case '\n': return "ENTER";
    case '\r': return "ENTER";
    case '\t': return "TAB";
    case 0x08: return "BACKSPACE";
    case '-': return "MINUS";
    case '=': return "EQUALS";
    case '[': return "LEFTBRACKET";
    case ']': return "RIGHTBRACKET";
    case ';': return "SEMICOLON";
    case '\'': return "APOSTROPHE";
    case ',': return "COMMA";
    case '.': return "PERIOD";
    case '/': return "SLASH";
    case '\\': return "BACKSLASH";
    default:
        // shifted punctuation (basic set)
        *needsShift = true;
        switch (c.unicode()) {
        case '!': return "1";
        case '@': return "2";
        case '#': return "3";
        case '$': return "4";
        case '%': return "5";
        case '^': return "6";
        case '&': return "7";
        case '*': return "8";
        case '(': return "9";
        case ')': return "0";
        case '_': return "MINUS";
        case '+': return "EQUALS";
        case '{': return "LEFTBRACKET";
        case '}': return "RIGHTBRACKET";
        case ':': return "SEMICOLON";
        case '"': return "APOSTROPHE";
        case '<': return "COMMA";
        case '>': return "PERIOD";
        case '?': return "SLASH";
        case '|': return "BACKSLASH";
        default:
            return QString(); // unsupported
        }
    }
}

bool DosboxRunner::sendText(const QString &text)
{
    for (QChar c : text) {
        bool shift = false;
        const QString key = keyNameForChar(c, &shift);
        if (key.isEmpty())
            continue;

        if (shift) sendKeyDown("LSHIFT");
        sendKey(key);
        if (shift) sendKeyUp("LSHIFT");
    }
    return true;
}

bool DosboxRunner::pressKey(const QString &keyName)
{
    // Civilization will typically respond to key presses, not text.
    if (!sendControlLine(QStringLiteral("KEYDOWN %1").arg(keyName)))
        return false;
    return sendControlLine(QStringLiteral("KEYUP %1").arg(keyName));
}

bool DosboxRunner::typeText(const QString &text)
{
    // Optional, may not work for Civ’s input model, but useful for later.
    // Escape newlines to avoid breaking protocol.
    QString t = text;
    t.replace('\n', ' ');
    t.replace('\r', ' ');
    return sendControlLine(QStringLiteral("TEXT %1").arg(t));
}

bool DosboxRunner::mouseLeftClick()
{
    if (!sendControlLine(QStringLiteral("MOUSEBTN left down")))
        return false;
    return sendControlLine(QStringLiteral("MOUSEBTN left up"));
}

bool DosboxRunner::mouseMove(int dx, int dy)
{
    return sendControlLine(QStringLiteral("MOUSEMOVE %1 %2").arg(dx).arg(dy));
}

void DosboxRunner::sendStartupSelections()
{
    // If process died, do nothing
    if (m_proc.state() != QProcess::Running)
        return;

    auto sendDigit = [this](int d) {
        // Your control socket expects KEYDOWN/KEYUP or the convenience "KEY <name>"
        // You said KEY "1", "Return", "Space" works, so use KEY here.
        sendControlLine(QString("KEY %1").arg(d));
    };

    const int g = qBound(1, m_startupGraphicsMode, 4);
    const int s = qBound(1, m_startupSoundMode, 6);
    const int c = qBound(1, m_startupControlMode, 2);

    // Civ menus accept the digit immediately (you tested sending "1" only).
    QTimer::singleShot(0,    this, [=]() { sendDigit(g); });
    QTimer::singleShot(600,  this, [=]() { sendDigit(s); });
    QTimer::singleShot(1200, this, [=]() { sendDigit(c); });
}

//static QSettings makeSettings()
//{
//    const QString cfgBase =
//            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
//    QDir().mkpath(cfgBase);
//    return QSettings(QDir(cfgBase).filePath("settings.ini"), QSettings::IniFormat);
//}

// Constructor
DosboxRunner::DosboxRunner(QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(civDir());
    loadSettings();
    rewriteConfigNow();
}

void DosboxRunner::rewriteConfigNow()
{
    QString err;
    // Only write if game is ready; otherwise don't overwrite with unusable mount
    if (!gameReady())
        return;

    writeConfigFile(&err);
    // If you want to debug:
    // if (!err.isEmpty()) qWarning() << "dosbox.conf write:" << err;
}

QString DosboxRunner::settingsIniPath() const
{
    const QString cfgBase =
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(cfgBase);
    return QDir(cfgBase).filePath(QStringLiteral("settings.ini"));
}

bool DosboxRunner::startupAutoSelect() const { return m_startupAutoSelect; }

void DosboxRunner::setStartupAutoSelect(bool v)
{
    if (m_startupAutoSelect == v) return;
    m_startupAutoSelect = v;
    saveSetting("startup/autoSelect", m_startupAutoSelect);
    emit startupAutoSelectChanged();
}

int DosboxRunner::startupGraphicsMode() const { return m_startupGraphicsMode; }

void DosboxRunner::setStartupGraphicsMode(int v)
{
    v = qBound(1, v, 4);
    if (m_startupGraphicsMode == v) return;
    m_startupGraphicsMode = v;
    saveSetting("startup/graphicsMode", m_startupGraphicsMode);
    emit startupGraphicsModeChanged();
}

int DosboxRunner::startupSoundMode() const { return m_startupSoundMode; }

void DosboxRunner::setStartupSoundMode(int v)
{
    v = qBound(1, v, 6);
    if (m_startupSoundMode == v) return;
    m_startupSoundMode = v;
    saveSetting("startup/soundMode", m_startupSoundMode);
    emit startupSoundModeChanged();
}

int DosboxRunner::startupControlMode() const { return m_startupControlMode; }

void DosboxRunner::setStartupControlMode(int v)
{
    v = qBound(1, v, 2);
    if (m_startupControlMode == v) return;
    m_startupControlMode = v;
    saveSetting("startup/controlMode", m_startupControlMode);
    emit startupControlModeChanged();
}

void DosboxRunner::loadSettings()
{
    QSettings st(settingsIniPath(), QSettings::IniFormat);
    st.beginGroup(QStringLiteral("dosbox"));

    m_windowRes = st.value(QStringLiteral("windowRes"), QStringLiteral("original")).toString();
    m_cycles    = st.value(QStringLiteral("cycles"),    QStringLiteral("auto")).toString();
    m_scaler    = st.value(QStringLiteral("scaler"),    QStringLiteral("normal2x")).toString();

    m_startupAutoSelect   = st.value(QStringLiteral("startup/autoSelect"), true).toBool();
    m_startupGraphicsMode = st.value(QStringLiteral("startup/graphicsMode"), 1).toInt();
    m_startupSoundMode    = st.value(QStringLiteral("startup/soundMode"), 4).toInt();
    m_startupControlMode  = st.value(QStringLiteral("startup/controlMode"), 1).toInt();

    st.endGroup();

    // Clamp to valid ranges
    m_startupGraphicsMode = qBound(1, m_startupGraphicsMode, 4);
    m_startupSoundMode    = qBound(1, m_startupSoundMode, 6);
    m_startupControlMode  = qBound(1, m_startupControlMode, 2);
}

void DosboxRunner::saveSetting(const QString &key, const QVariant &value) const
{
    QSettings st(settingsIniPath(), QSettings::IniFormat);
    st.beginGroup(QStringLiteral("dosbox"));
    st.setValue(key, value);
    st.endGroup();
    st.sync();
}

void DosboxRunner::setWindowRes(const QString &value)
{
    const QString v = value.trimmed();
    if (v.isEmpty() || v == m_windowRes)
        return;

    m_windowRes = v;
    saveSetting(QStringLiteral("windowRes"), m_windowRes);

    emit windowResChanged();
    rewriteConfigNow();
}

QString DosboxRunner::civDir() const
{
    // ~/.local/share/harbour-classicciv/civ
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath("civ");
}

bool DosboxRunner::gameReady() const
{
    return !findCivExePath().isEmpty();
}

void DosboxRunner::setCycles(const QString &v)
{
    const QString nv = v.trimmed();
    if (nv.isEmpty() || nv == m_cycles)
        return;

    m_cycles = nv;
    saveSetting(QStringLiteral("cycles"), m_cycles);

    emit settingsChanged();
    rewriteConfigNow();
}

void DosboxRunner::setScaler(const QString &v)
{
    const QString nv = v.trimmed();
    if (nv.isEmpty() || nv == m_scaler)
        return;

    m_scaler = nv;
    saveSetting(QStringLiteral("scaler"), m_scaler);

    emit settingsChanged();
    rewriteConfigNow();
}

void DosboxRunner::rescan()
{
    emit civDirChanged();
    emit gameReadyChanged();
}

QString DosboxRunner::appShareBinDosboxPath() const
{
    // Installed by RPM into /usr/share/harbour-classicciv/bin/dosbox
    return QStringLiteral("/usr/share/harbour-classicciv/bin/dosbox");
}

QString DosboxRunner::configPath() const
{
    // ~/.config/harbour-classicciv/dosbox.conf
    const QString cfgBase = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(cfgBase);
    return QDir(cfgBase).filePath("dosbox.conf");
}

bool DosboxRunner::fileExistsCaseInsensitive(const QString &dirPath, const QString &fileName, QString *outActualName)
{
    QDir dir(dirPath);
    const auto entries = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QString &e : entries) {
        if (QString::compare(e, fileName, Qt::CaseInsensitive) == 0) {
            if (outActualName) *outActualName = e;
            return true;
        }
    }
    return false;
}

QString DosboxRunner::findCivExePath() const
{
    QString actual;
    if (!fileExistsCaseInsensitive(civDir(), expectedExe(), &actual))
        return QString();
    return QDir(civDir()).filePath(actual);
}

bool DosboxRunner::writeConfigFile(QString *outError) const
{
    if (!gameReady()) {
        if (outError) *outError = QStringLiteral("CIV.EXE not found in civ directory.");
        return false;
    }

    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (outError) *outError = QStringLiteral("Failed to write dosbox.conf");
        return false;
    }

    QTextStream s(&f);

    // --- SDL / video ---
    s << "[sdl]\n";
    s << "fullscreen=" << (m_fullscreen ? "true" : "false") << "\n";
    s << "fullresolution=" << (m_fullscreenRes.isEmpty() ? "desktop" : m_fullscreenRes) << "\n";
    s << "windowresolution=" << (m_windowRes.isEmpty() ? "desktop" : m_windowRes) << "\n";
    s << "output=texture\n";
    s << "\n";

    // --- Render ---
    s << "[render]\n";
    s << "aspect=true\n";
    s << "integer_scaling=" << (m_integerScaling ? "on" : "off") << "\n";
    if (!m_viewport.isEmpty())
        s << "viewport=" << m_viewport << "\n";
    // Keep for now; still accepted but deprecated (your warning confirms this)
    s << "scaler=" << (m_scaler.isEmpty() ? "normal2x" : m_scaler) << "\n";
    s << "\n";

    // --- Mouse (sensitivity moved here) ---
    s << "[mouse]\n";
    s << "mouse_sensitivity=" << m_mouseSensitivity << "\n";
    s << "\n";

    // --- CPU (use cpu_cycles instead of cycles) ---
    s << "[cpu]\n";
    s << "core=auto\n";
    s << "cycles=" << (m_cycles.isEmpty() ? "auto" : m_cycles) << "\n";
    s << "\n";

    // --- Mixer ---
    s << "[mixer]\n";
    s << "nosound=false\n";
    s << "\n";

    // --- Autoexec ---
    s << "[autoexec]\n";
    s << "mount c \"" << civDir() << "\"\n";
    s << "c:\n";
    s << "CIV.EXE\n";
    s << "\n";

    return true;
}

bool DosboxRunner::launch()
{
    QString err;
    if (!writeConfigFile(&err)) {
        return false;
    }

    const QString dosbox = appShareBinDosboxPath();
    if (!QFileInfo::exists(dosbox)) {
        return false;
    }

    if (m_proc.state() != QProcess::NotRunning) {
        // already running
        return true;
    }

    QStringList args;
    args << "--noprimaryconf"
         << "--nolocalconf"
         << "--conf" << configPath()
         << "--noconsole";

    // Ensure process sees Civ directory
    m_proc.setWorkingDirectory(civDir());

    // Optional: expose touch parameters to your patched DOSBox via env vars.
    // (Patch reads these if present; see section 6.)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString sock = QDir::temp().filePath("harbour-classicciv.sock");
    env.insert("DOSBOX_CONTROL_SOCKET", sock);
    env.insert("HARBOUR_CIV_LONGPRESS_MS", "450");
    env.insert("HARBOUR_CIV_TAP_MAX_MS", "220");
    env.insert("HARBOUR_CIV_JITTER_PX", "18");
    m_proc.setProcessEnvironment(env);

    m_proc.start(dosbox, args);
//    return m_proc.waitForStarted(2000);
    const bool started = m_proc.waitForStarted(2000);
    if (started && m_startupAutoSelect) {
        // Give DOSBox/Civ a moment to reach the first menu
        QTimer::singleShot(900, this, [this]() { sendStartupSelections(); });
    }
    return started;
}

void DosboxRunner::openCivFolderInFileManager()
{
    // Open folder in default handler (file manager)
    QDesktopServices::openUrl(QUrl::fromLocalFile(civDir()));
}
