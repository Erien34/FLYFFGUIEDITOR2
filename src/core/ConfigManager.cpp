#include "ConfigManager.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QDebug>

// --------------------------------------------------
// Konstruktor
// --------------------------------------------------
ConfigManager::ConfigManager() = default;

// --------------------------------------------------
// Standardpfade
// --------------------------------------------------
QString ConfigManager::defaultConfigPath()
{
    const QString base = QCoreApplication::applicationDirPath();
    return base + "/config/config.ini";
}

QString ConfigManager::defaultConfigDir()
{
    const QString base = QCoreApplication::applicationDirPath();
    return base + "/config";
}

QString ConfigManager::windowFlagsPath() const {
    return defaultConfigDir() + "/window_flags.json";
}

QString ConfigManager::controlFlagsPath() const {
    return defaultConfigDir() + "/control_flags.json";
}

// --------------------------------------------------
// Config laden
// --------------------------------------------------
bool ConfigManager::load(const QString& filePath)
{
    if (!QFileInfo::exists(filePath)) {
        qWarning() << "[ConfigManager] Config-Datei fehlt:" << filePath;
        return false;
    }

    QSettings cfg(filePath, QSettings::IniFormat);

    m_layoutPath  = cfg.value("Paths/LayoutPath").toString();
    m_themePath   = cfg.value("Paths/ThemePath").toString();
    m_iconPath    = cfg.value("Paths/IconPath").toString();
    m_sourcePath  = cfg.value("Paths/SourcePath").toString();

    // 🔹 Flags und Regeln
    m_windowFlagsPath        = cfg.value("Flags/WindowFlags").toString();
    m_controlFlagsPath       = cfg.value("Flags/ControlFlags").toString();
    m_undefinedControlFlagsPath = cfg.value("Flags/UndefinedControlFlags").toString();
    m_flagWindowRulesPath    = cfg.value("Flags/FlagWindowRules").toString();
    m_flagControlRulesPath  = cfg.value("Flags/FlagControlRules").toString();

    bool updated = false;

    // Fallbacks ergänzen, falls leer
    if (m_windowFlagsPath.isEmpty()) {
        m_windowFlagsPath = defaultConfigDir() + "/window_flags.json";
        cfg.setValue("Flags/WindowFlags", m_windowFlagsPath);
        updated = true;
    }

    if (m_controlFlagsPath.isEmpty()) {
        m_controlFlagsPath = defaultConfigDir() + "/control_flags.json";
        cfg.setValue("Flags/ControlFlags", m_controlFlagsPath);
        updated = true;
    }

    if (m_flagWindowRulesPath.isEmpty()) {
        m_flagWindowRulesPath = defaultConfigDir() + "/window_flag_rules.json";
        cfg.setValue("Flags/FlagWindowRules", m_flagWindowRulesPath);
        updated = true;
    }

    if (m_flagControlRulesPath.isEmpty()) {
        m_flagControlRulesPath = defaultConfigDir() + "/control_flag_rules.json";
        cfg.setValue("Flags/FlagControlRules", m_flagControlRulesPath);
        updated = true;
    }

    if (m_undefinedControlFlagsPath.isEmpty()) {
        m_undefinedControlFlagsPath = defaultConfigDir() + "/control_flags_undefined.json";
        cfg.setValue("Flags/UndefinedControlFlags", m_undefinedControlFlagsPath);
        updated = true;
    }

    if (updated) {
        cfg.sync();
        qInfo() << "[ConfigManager] Fehlende Flag-Pfade ergänzt und gespeichert.";
    }

    qInfo() << "[ConfigManager] Geladen:" << filePath;
    return true;
}

// --------------------------------------------------
// Config speichern
// --------------------------------------------------
bool ConfigManager::save(const QString& filePath) const
{
    QDir dir(QFileInfo(filePath).absolutePath());
    if (!dir.exists())
        dir.mkpath(".");

    QSettings cfg(filePath, QSettings::IniFormat);

    cfg.setValue("Paths/LayoutPath", m_layoutPath);
    cfg.setValue("Paths/ThemePath",  m_themePath);
    cfg.setValue("Paths/IconPath",   m_iconPath);
    cfg.setValue("Paths/SourcePath", m_sourcePath);

    // 🔹 Flags
    cfg.setValue("Flags/WindowFlags",  m_windowFlagsPath);
    cfg.setValue("Flags/ControlFlags", m_controlFlagsPath);
    cfg.setValue("Flags/UndefinedControlFlags", m_undefinedControlFlagsPath);

    // 🔹 Neue Regel-Dateien
    cfg.setValue("Flags/FlagWindowRules",   m_flagWindowRulesPath);
    cfg.setValue("Flags/FlagControlRules",  m_flagControlRulesPath);

    cfg.sync();
    qInfo() << "[ConfigManager] Gespeichert:" << filePath;
    return true;
}

// --------------------------------------------------
// Default config anlegen (wenn sie fehlt)
// --------------------------------------------------
bool ConfigManager::createDefault(const QString& filePath)
{
    qInfo() << "[ConfigManager] Erstelle Standard-Konfiguration:" << filePath;

    const QString base = QCoreApplication::applicationDirPath();
    const QString cfgDir = base + "/config";

    QDir().mkpath(cfgDir);

    m_layoutPath = base + "/data/layouts";
    m_themePath  = base + "/data/themes";
    m_iconPath   = base + "/data/icons";
    m_sourcePath = base + "/data/source";

    // 🔹 Flags + Regeln
    m_windowFlagsPath        = cfgDir + "/window_flags.json";
    m_controlFlagsPath       = cfgDir + "/control_flags.json";
    m_undefinedControlFlagsPath       = cfgDir + "/control_flags_undefined.json";

    m_flagWindowRulesPath    = cfgDir + "/window_flag_rules.json";
    m_flagControlRulesPath  = cfgDir + "/control_flag_rules.json";

    return save(filePath);
}

// --------------------------------------------------
// Automatisch laden oder neue Config erstellen
// --------------------------------------------------
bool ConfigManager::loadOrCreate()
{
    const QString path = defaultConfigPath();

    if (QFileInfo::exists(path))
        return load(path);

    return createDefault(path);
}
