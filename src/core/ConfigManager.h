#pragma once
#include <QString>
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

class ConfigManager
{
public:
    ConfigManager();

    // Standardpfad der config.ini
    static QString defaultConfigPath();
    static QString defaultConfigDir();

    // Laden & Speichern
    bool load(const QString& filePath);
    bool save(const QString& filePath) const;
    bool createDefault(const QString& filePath);
    bool loadOrCreate();

    // Getter / Setter für Pfade
    QString layoutPath() const { return m_layoutPath; }
    QString themePath() const  { return m_themePath; }
    QString iconPath() const   { return m_iconPath; }
    QString sourcePath() const { return m_sourcePath; }
    QString flagWindowRulesPath() const { return m_flagWindowRulesPath; }
    QString flagControlRulesPath() const { return m_flagControlRulesPath; }

    QString undefinedControlFlagsPath() const;
    QString windowFlagsPath() const;
    QString controlFlagsPath() const;

    void setLayoutPath(const QString& v) { m_layoutPath = v; }
    void setThemePath(const QString& v)  { m_themePath = v; }
    void setIconPath(const QString& v)   { m_iconPath = v; }
    void setSourcePath(const QString& v) { m_sourcePath = v; }

    void setWindowFlagsPath(const QString& v)  { m_windowFlagsPath = v; }
    void setControlFlagsPath(const QString& v) { m_controlFlagsPath = v; }
    void flagWindowRulesPath(const QString& v) { m_flagWindowRulesPath = v; }
    void flagControlRulesPath(const QString& v) { m_flagControlRulesPath = v; }
    void setUndefinedControlFlagsPath(const QString& v) { m_undefinedControlFlagsPath = v; }

private:
    QString m_layoutPath;
    QString m_themePath;
    QString m_iconPath;
    QString m_sourcePath;
    QString m_flagWindowRulesPath;
    QString m_flagControlRulesPath;

    QString m_windowFlagsPath;
    QString m_controlFlagsPath;
    QString m_undefinedControlFlagsPath;
};
