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

    /**
     * @brief Startet die Flaggenerierung:
     *        - durchsucht alle Header-Dateien im Source-Ordner
     *        - parst #define-Makros
     *        - speichert JSON-Dateien in den angegebenen Pfaden
     *
     * @param wndPath Pfad zur window_flags.json
     * @param ctrlPath Pfad zur control_flags.json
     */
    void generateFlags(const QString& sourceDir,
                        const QString& wndPath,
                        const QString& ctrlPath);

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
};

