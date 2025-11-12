#pragma once

#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <QString>
#include <QFlags>
#include <memory>
#include <vector>

// ---------------------------------------------------------
// Beschreibende Struktur für fertiges Verhalten
// ---------------------------------------------------------
struct BehaviorInfo {
    QString category;                      // z.B. "button", "label", "listbox"
    QMap<QString, QVariant> attributes;    // beliebige Meta-Infos
};

// ---------------------------------------------------------
// Grundfähigkeiten eines Controls (hartcodiert)
// ---------------------------------------------------------
enum ControlCapability : quint32
{
    ControlCapability_None           = 0,
    ControlCapability_CanClick       = 1 << 0,
    ControlCapability_CanToggle      = 1 << 1,
    ControlCapability_CanFocus       = 1 << 2,
    ControlCapability_CanTextInput   = 1 << 3,
    ControlCapability_CanSelectItems = 1 << 4,
    ControlCapability_CanScroll      = 1 << 5,
    ControlCapability_IsContainer    = 1 << 6,
    ControlCapability_HasTooltip     = 1 << 7,
    ControlCapability_CustomBehavior = 1 << 8
};
Q_DECLARE_FLAGS(ControlCapabilities, ControlCapability)
Q_DECLARE_OPERATORS_FOR_FLAGS(ControlCapabilities)

// ---------------------------------------------------------
// Basisverhalten pro Typ (hartcodiert)
// ---------------------------------------------------------
struct BaseBehavior
{
    QString             category;       // z.B. "button", "label"
    ControlCapabilities capabilities;   // Grundfähigkeiten (CanClick, ...)
    QMap<QString, QVariant> defaults;   // z.B. defaultColor, textAlign, ...
};

// Forward-Declarations
class FlagManager;
class TextManager;
class DefineManager;
class LayoutManager;
class LayoutBackend;
struct ControlData;
struct WindowData;

// =========================================================
// BehaviorManager
//  - verwaltet Flags & Flag-Regeln
//  - hält Basis-Behavior pro Control-Typ
//  - liefert auf Anfrage BehaviorInfo für Renderer/Input
// =========================================================
class BehaviorManager
{
public:
    BehaviorManager(FlagManager*   flagMgr,
                    TextManager*   textMgr,
                    DefineManager* defineMgr,
                    LayoutManager* layoutMgr,
                    LayoutBackend* layoutBackend);

    // -----------------------------------------
    // 🔹 Flags aus JSON laden (Fenster / Controls)
    // -----------------------------------------
    void refreshFlagsFromFiles(const QString& wndFlagsPath = QString(),
                               const QString& ctrlFlagsPath = QString());

    const QMap<QString, quint32>& windowFlags()  const { return m_windowFlags;  }
    const QMap<QString, quint32>& controlFlags() const { return m_controlFlags; }

    // -----------------------------------------
    // 🔹 Flag-Regeln (window_flag_rules.json / control_flag_rules.json)
    // -----------------------------------------
    QJsonObject windowFlagRules() const;
    QJsonObject controlFlagRules() const;

    // -----------------------------------------
    // 🔹 Masken → resolvedMask aktualisieren
    // -----------------------------------------
    void updateWindowFlags(const std::shared_ptr<WindowData>& wnd) const;
    void updateControlFlags(const std::shared_ptr<ControlData>& ctrl) const;
    void applyWindowStyle(WindowData& wnd) const;

    // -----------------------------------------
    // 🔹 Validierung & Analyse (LayoutManager ruft das)
    // -----------------------------------------
    void validateWindowFlags(WindowData* wnd) const;
    void validateControlFlags(ControlData* ctrl) const;
    void analyzeControlTypes(const std::vector<std::shared_ptr<WindowData>>& windows) const;
    void generateUnknownControls(const std::vector<std::shared_ptr<WindowData>>& windows) const;

    // -----------------------------------------
    // 🔹 Behavior-API
    //     - kombiniert BaseBehavior (hartcodiert)
    //       + optionale Config aus Datei
    // -----------------------------------------
    BehaviorInfo resolveBehavior(const ControlData& ctrl) const;
    void applyBehavior(ControlData& ctrl) const;   // optional: Defaults anwenden

private:
    FlagManager*    m_flagMgr   = nullptr;
    TextManager*    m_textMgr   = nullptr;
    DefineManager*  m_defineMgr = nullptr;
    LayoutManager*  m_layoutMgr = nullptr;
    LayoutBackend*  m_layoutBackend = nullptr;

    // Flags (aus window_flags.json / control_flags.json)
    QMap<QString, quint32> m_windowFlags;
    QMap<QString, quint32> m_controlFlags;

    // Regeln (aus window_flag_rules.json / control_flag_rules.json)
    mutable QJsonObject m_windowRules;
    mutable QJsonObject m_controlRules;
    mutable bool m_windowRulesLoaded  = false;
    mutable bool m_controlRulesLoaded = false;

    // Basis-Behavior (hartcodiert) pro Control-Type (z.B. "WTYPE_BUTTON")
    QMap<QString, BaseBehavior> m_baseBehaviors;

    // Erweiterbare Behavior-Konfiguration aus Datei (optional)
    mutable QJsonObject m_behaviorConfig;
    mutable bool m_behaviorConfigLoaded = false;

    // Initialisierungen / Lazy-Loader
    void initializeBaseBehaviors();
    void reloadWindowFlagRules() const;
    void reloadControlFlagRules() const;
    void reloadBehaviorConfig() const;
};
