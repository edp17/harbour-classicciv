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

QString DosboxRunner::controlSocketPath() const
{
    // Keep in sync with the DOSBox-side socket path you used.
    return QStringLiteral("/tmp/harbour-classicciv.sock");
}

bool DosboxRunner::sendControlLine(const QString &line)
{
    return send_to_control_socket(controlSocketPath(), line);
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

static QSettings makeSettings()
{
    const QString cfgBase =
            QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(cfgBase);
    return QSettings(QDir(cfgBase).filePath("settings.ini"), QSettings::IniFormat);
}

// Constructor
DosboxRunner::DosboxRunner(QObject *parent)
    : QObject(parent)
{
    QDir().mkpath(civDir());

    QSettings st = makeSettings();
    m_cycles    = st.value(QStringLiteral("dosbox/cycles"),
                           QStringLiteral("auto")).toString();
    m_scaler    = st.value(QStringLiteral("dosbox/scaler"),
                           QStringLiteral("normal2x")).toString();
    m_windowRes = st.value(QStringLiteral("dosbox/windowRes"),
                           QStringLiteral("original")).toString();
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

void DosboxRunner::loadSettings()
{
    QSettings st(settingsIniPath(), QSettings::IniFormat);
    st.beginGroup(QStringLiteral("dosbox"));
    m_windowRes = st.value(QStringLiteral("windowRes"), QStringLiteral("original")).toString();
    m_cycles    = st.value(QStringLiteral("cycles"),    QStringLiteral("auto")).toString();
    m_scaler    = st.value(QStringLiteral("scaler"),    QStringLiteral("normal2x")).toString();
    st.endGroup();
}

void DosboxRunner::saveSetting(const QString &key, const QVariant &value) const
{
    QSettings st(settingsIniPath(), QSettings::IniFormat);
    st.beginGroup(QStringLiteral("dosbox"));
    st.setValue(key, value);
    st.endGroup();
    st.sync();
}

QString DosboxRunner::windowRes() const
{
    return m_windowRes;
}

void DosboxRunner::setWindowRes(const QString &value)
{
    const QString v = value.trimmed();
    if (v.isEmpty() || v == m_windowRes)
        return;

    m_windowRes = v;

    QSettings st = makeSettings();
    st.setValue(QStringLiteral("dosbox/windowRes"), m_windowRes);

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

    QSettings st = makeSettings();
    st.setValue(QStringLiteral("dosbox/cycles"), m_cycles);

    emit settingsChanged();
    rewriteConfigNow();
}

void DosboxRunner::setScaler(const QString &v)
{
    const QString nv = v.trimmed();
    if (nv.isEmpty() || nv == m_scaler)
        return;

    m_scaler = nv;

    QSettings st = makeSettings();
    st.setValue(QStringLiteral("dosbox/scaler"), m_scaler);

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
    return m_proc.waitForStarted(2000);
}

void DosboxRunner::openCivFolderInFileManager()
{
    // Open folder in default handler (file manager)
    QDesktopServices::openUrl(QUrl::fromLocalFile(civDir()));
}
