#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <memory>

class ConfigManager;

/**
 * @brief Der FlagManager durchsucht die Source-Dateien nach #define-Flags
 *        (z. B. WBS_*, WTYPE_*, WLVS_*, EBS_*, WIDC_*),
 *        und erzeugt daraus mehrere JSON-Dateien:
 *        - window_flags.json   → Bitmasken für Fenster
 *        - control_flags.json  → Bitmasken für Controls
 *        - window_types.json   → Fenstertyp-Enums (WTYPE_*)
 *
 *  Außerdem ergänzt er fehlende Standard-Flags automatisch.
 */
class FlagManager : public QObject
{
    Q_OBJECT

public:
    explicit FlagManager(ConfigManager* config = nullptr);

    bool loadLegacyFlags(const QString& baseDir);
    bool useLegacyMode(bool enabled);
    const QMap<QString, QString>& legacyWindowFlags() const { return m_legacyWindowFlags; }
    const QMap<QString, QString>& legacyControlFlags() const { return m_legacyControlFlags; }
    const QMap<QString, QString>& windowFlags() const { return m_windowFlags; }
    const QMap<QString, QString>& controlFlags() const { return m_controlFlags; }

    void generateFlags(const QString& sourceDir,
                        const QString& wndPath,
                        const QString& ctrlPath);
    void generateRuleTemplates(const QString& configDir);
    void extendRuleFile(const QString& filePath,
                                     const QMap<QString, QString>& knownFlags);
    void initDefaultSemantics();
    void autoFillSemantics(const QString& ruleFilePath);
    void extendFlagGroups(const QString& groupsPath);
    void generateFlagGroups(const QString& configDir);

private:
    // ------------------------------------------------------------
    // Hilfsfunktionen
    // ------------------------------------------------------------

    /// durchsucht eine Header-Datei nach relevanten Defines
    void parseHeaderFile(const QString& filePath);

    /// ergänzt Standard-Fenster-Flags (falls im Code nicht gefunden)
    void applyDefaultWindowFlags();

    /// ergänzt Standard-Fenster-Typen (WTYPE_*)
    void applyDefaultWindowTypes();
    void applyDefaultControlFlags();

    /// speichert beliebige Key/Value-Maps als JSON
    void saveFlags(const QString& configDir);
    void saveJsonFile(const QString& path, const QMap<QString, QString>& data, const QString& label);

private:
    ConfigManager* m_config = nullptr;

    // ------------------------------------------------------------
    // Datensammlungen
    // ------------------------------------------------------------
    QMap<QString, QString> m_windowFlags;   ///< z. B. WBS_*, WBS_HELP etc.
    QMap<QString, QString> m_controlFlags;  ///< z. B. WLVS_*, EBS_*, WIDC_*
    QMap<QString, QString> m_windowTypes;   ///< z. B. WTYPE_BASE, WTYPE_BUTTON etc.
    QMap<QString, QString> m_unknownFlags;
    QMap<QString, QString> m_legacyWindowFlags;
    QMap<QString, QString> m_legacyControlFlags;
    QStringList m_legacyOverrides;
    QMap<QString, QJsonObject> m_defaultSemantics;

    bool m_useLegacy = false;
};

