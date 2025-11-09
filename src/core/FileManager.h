#pragma once
#include <QString>
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include "ConfigManager.h"

class FileManager
{
public:
    explicit FileManager(ConfigManager* cfg = nullptr) : m_config(cfg) {}
    void setConfig(ConfigManager* cfg) { m_config = cfg; }

    QString layoutPath() const;
    QString definePath() const;
    QString textPath() const;

    QString themePath() const { return m_config ? m_config->themePath() : QString(); }
    QString iconPath()  const { return m_config ? m_config->iconPath()  : QString(); }
    QString sourcePath() const { return m_config ? m_config->sourcePath() : QString(); }

    bool hasCachedFlags(const QString& configDir) const;
    QString findTextFile(const QString& baseDir) const;
    QString findTextIncFile(const QString& layoutFile) const;
    QString findDefineFile(const QString& baseDir) const;

    // JSON-Dateien
    QJsonObject loadJsonObject(const QString& filePath) const;
    bool saveJsonObject(const QString& filePath, const QJsonObject& json) const;

    // Flag-/Regel-Pfade
    QString windowFlagsPath() const;
    QString controlFlagsPath() const;
    QString windowTypesPath() const;
    QString undefinedControlFlagsPath() const;
    QString windowFlagRulesPath() const;
    QString controlFlagRulesPath() const;

    // Undefinierte ControlFlags
    static bool loadUndefinedControlFlags(const ConfigManager& cfg, QJsonObject& outJson);
    static bool saveUndefinedControlFlags(const ConfigManager& cfg, const QJsonObject& json);

private:
    ConfigManager* m_config = nullptr;
    QString m_layoutPath;
    mutable QString m_definePath;
    mutable QString m_textPath;
};
