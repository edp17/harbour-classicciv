#include "DosboxRunner.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTextStream>
#include <QDesktopServices>
#include <QUrl>

DosboxRunner::DosboxRunner(QObject *parent)
    : QObject(parent)
{
    // Ensure civ dir exists
    QDir().mkpath(civDir());
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

    // Minimal, safe defaults. Tune later.
    s << "[sdl]\n";
    s << "fullscreen=true\n";
    s << "fulldouble=false\n";
    s << "fullresolution=original\n";
    s << "windowresolution=original\n";
    s << "output=texture\n";
    s << "autolock=true\n";
    s << "sensitivity=100\n";
    s << "\n";

    s << "[render]\n";
    s << "aspect=true\n";
    s << "scaler=" << m_scaler << "\n";
    s << "\n";

    s << "[cpu]\n";
    s << "core=auto\n";
    s << "cycles=" << m_cycles << "\n";
    s << "\n";

    s << "[mixer]\n";
    s << "nosound=false\n";
    s << "\n";

    s << "[autoexec]\n";
    // DOSBox expects host paths; quoting helps with spaces.
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
