#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class DosboxRunner : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString civDir READ civDir NOTIFY civDirChanged)
    Q_PROPERTY(QString expectedExe READ expectedExe CONSTANT)
    Q_PROPERTY(bool gameReady READ gameReady NOTIFY gameReadyChanged)

    // Simple settings for v0.1
    Q_PROPERTY(QString cycles READ cycles WRITE setCycles NOTIFY settingsChanged)
    Q_PROPERTY(QString scaler READ scaler WRITE setScaler NOTIFY settingsChanged)
    Q_PROPERTY(QString windowRes READ windowRes WRITE setWindowRes NOTIFY windowResChanged)

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

    QString windowRes() const;
    void setWindowRes(const QString &value);

    Q_INVOKABLE bool pressKey(const QString &keyName);
    Q_INVOKABLE bool typeText(const QString &text);
    Q_INVOKABLE bool mouseLeftClick();
    Q_INVOKABLE bool mouseMove(int dx, int dy);
    Q_INVOKABLE bool sendControlLine(const QString &line); // useful for debugging

signals:
    void civDirChanged();
    void gameReadyChanged();
    void settingsChanged();
    void lastErrorChanged();
    void windowResChanged();

private:
    QString appShareBinDosboxPath() const;
    QString configPath() const;
    QString findCivExePath() const;

    bool writeConfigFile(QString *outError = nullptr) const;
    static bool fileExistsCaseInsensitive(const QString &dirPath, const QString &fileName, QString *outActualName = nullptr);

    QString settingsIniPath() const;
    void loadSettings();
    void saveSetting(const QString &key, const QVariant &value) const;
    void rewriteConfigNow();

private:
    mutable QProcess m_proc;
    // Video / scaling
    bool m_fullscreen = false;
    QString m_fullscreenRes;        // empty -> default to "desktop"
    QString m_windowRes = "original";
    bool m_integerScaling = false;
    QString m_viewport;             // empty means don't write it

    // Legacy / compatibility (keep for now)
    QString m_scaler = "normal2x";  // still accepted but deprecated
    QString m_cycles = "auto";      // deprecated but accepted

    // Mouse
    int m_mouseSensitivity = 100;

    QString controlSocketPath() const;

};
