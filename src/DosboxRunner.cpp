#include "DosboxRunner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>
#include <QSettings>

// Constructor
DosboxRunner::DosboxRunner(QObject *parent)
    : QObject(parent)
{
    // Ensure civ dir exists
    QDir().mkpath(civDir());

    QSettings st;
    m_windowRes = st.value(QStringLiteral("dosbox/windowRes"), QStringLiteral("original")).toString();
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

    QSettings st;
    st.setValue(QStringLiteral("dosbox/windowRes"), m_windowRes);

    emit windowResChanged();
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
    if (m_cycles == v) return;
    m_cycles = v.trimmed();
    emit settingsChanged();
}

void DosboxRunner::setScaler(const QString &v)
{
    if (m_scaler == v) return;
    m_scaler = v.trimmed();
    emit settingsChanged();
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
    s << "fullscreen=true\n";
    s << "fullresolution=original\n";
    // dosbox-staging recommends windowresolution/viewport instead of software scalers
    s << "windowresolution=" << m_windowRes << "\n";   // e.g. "original", "desktop", "1280x720"
    s << "output=texture\n";
    s << "\n";

    // --- Render ---
    s << "[render]\n";
    s << "aspect=true\n";
    // Optional but useful on phones; add a bool property later if you want a toggle:
    // s << "integer_scaling=true\n";
    s << "\n";

    // --- Mouse (sensitivity moved here) ---
    s << "[mouse]\n";
    s << "mouse_sensitivity=100\n";
    s << "\n";

    // --- CPU (use cpu_cycles instead of cycles) ---
    s << "[cpu]\n";
    s << "core=auto\n";
    s << "cpu_cycles=" << m_cycles << "\n"; // allow "auto" or a number string
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
    args << "-conf" << configPath();
    args << "-noconsole";

    // Ensure process sees Civ directory
    m_proc.setWorkingDirectory(civDir());

    // Optional: expose touch parameters to your patched DOSBox via env vars.
    // (Patch reads these if present; see section 6.)
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
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
