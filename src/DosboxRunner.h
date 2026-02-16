#pragma once

#include <QObject>
#include <QProcess>

class DosboxRunner : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString civDir READ civDir NOTIFY civDirChanged)
    Q_PROPERTY(QString expectedExe READ expectedExe CONSTANT)
    Q_PROPERTY(bool gameReady READ gameReady NOTIFY gameReadyChanged)

    // Simple settings for v0.1
    Q_PROPERTY(QString cycles READ cycles WRITE setCycles NOTIFY settingsChanged)
    Q_PROPERTY(QString scaler READ scaler WRITE setScaler NOTIFY settingsChanged)

public:
    explicit DosboxRunner(QObject *parent = nullptr);

    QString civDir() const;
    QString expectedExe() const { return QStringLiteral("CIV.EXE"); }
    bool gameReady() const;

    QString cycles() const { return m_cycles; }
    void setCycles(const QString &v);

    QString scaler() const { return m_scaler; }
    void setScaler(const QString &v);

    Q_INVOKABLE void rescan();
    Q_INVOKABLE bool launch();
    Q_INVOKABLE void openCivFolderInFileManager();

signals:
    void civDirChanged();
    void gameReadyChanged();
    void settingsChanged();
    void lastErrorChanged();

private:
    QString appShareBinDosboxPath() const;
    QString configPath() const;
    QString findCivExePath() const;

    bool writeConfigFile(QString *outError = nullptr) const;
    static bool fileExistsCaseInsensitive(const QString &dirPath, const QString &fileName, QString *outActualName = nullptr);

private:
    mutable QProcess m_proc;
    QString m_cycles = QStringLiteral("auto");       // or "3000", etc.
    QString m_scaler = QStringLiteral("normal2x");   // good default for phones
};
