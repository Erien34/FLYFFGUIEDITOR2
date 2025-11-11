#pragma once

#include <QMap>
#include <QSet>
#include <QJsonObject>
#include <QVariant>
#include <QString>
#include <memory>
#include <vector>

// ----------------------------------------------------------
// BehaviorInfo – High-Level-Infos zu einem Control-Behavior
// ----------------------------------------------------------
struct BehaviorInfo {
    QString category;                      // optional (z.B. "Button", "List", ...)
    QMap<QString, QVariant> attributes;    // optionale Attribute (z.B. "isToggle", "hasTooltip", ...)
};

// Forward-Declarations
class FlagManager;
class TextManager;
class DefineManager;
class LayoutManager;
struct ControlData;
struct WindowData;

// ================================================================
// BehaviorManager
//  - High-Level: resolveBehavior / applyBehavior (dein ursprüngliches API)
//  - Low-Level:  Flags, Rules, Validierung, UnknownBits, Analyse
// ================================================================
class BehaviorManager
{
public:
    BehaviorManager(FlagManager* flagMgr,
                    TextManager* textMgr,
                    DefineManager* defineMgr,
                    LayoutManager* layoutMgr);

    // ----------------------------------------------------------
    // 🔹 High-Level Behavior API (hattest du schon)
    // ----------------------------------------------------------
    BehaviorInfo resolveBehavior(const ControlData& ctrl) const;
    void applyBehavior(ControlData& ctrl) const;

    // ----------------------------------------------------------
    // 🔹 Flags & Types laden
    //    (ersetzt: LayoutManager::refreshFromFiles)
    // ----------------------------------------------------------
    void refreshFlagsFromFiles();

    // ----------------------------------------------------------
    // 🔹 Regeln & Default-Regeln
    //    (ersetzt: generateDefaultWindowFlagRules / Control)
    // ----------------------------------------------------------
    QJsonObject getWindowFlagRules() const;
    QJsonObject getControlFlagRules() const;

    void generateDefaultWindowFlagRules();
    void generateDefaultControlFlagRules();

    // ----------------------------------------------------------
    // 🔹 Allowed-Flags pro Typ
    //    (ersetzt: allowedWindowFlags / allowedControlFlags)
    // ----------------------------------------------------------
    QSet<QString> allowedWindowFlags(const QString& typeName) const;
    QSet<QString> allowedControlFlags(const QString& typeName) const;

    // ----------------------------------------------------------
    // 🔹 Validierung & Updates
    //    (ersetzt: validateWindowFlags/ControlFlags + update*)
    // ----------------------------------------------------------
    void validateWindowFlags(WindowData* win);
    void validateControlFlags(ControlData* ctrl);

    void updateWindowFlags(const std::shared_ptr<WindowData>& wnd);
    void updateControlFlags(const std::shared_ptr<ControlData>& ctrl);

    // ----------------------------------------------------------
    // 🔹 Analyse / Meta
    //    (ersetzt: analyzeControlTypes / allowsPropertyForType)
    // ----------------------------------------------------------
    void analyzeControlTypes(const std::vector<std::shared_ptr<WindowData>>& windows);
    bool allowsPropertyForType(const QString& type, const QString& property) const;

    // ----------------------------------------------------------
    // 🔹 Unknown Control Bits
    //    (ersetzt: generateUnknownControls)
    // ----------------------------------------------------------
    void generateUnknownControls(const std::vector<std::shared_ptr<WindowData>>& windows);

    // ----------------------------------------------------------
    // 🔹 Optionale Getter für Editor/Debug
    // ----------------------------------------------------------
    const QMap<QString, quint32>& windowFlags()  const { return m_windowFlags; }
    const QMap<QString, quint32>& controlFlags() const { return m_controlFlags; }
    const QMap<QString, quint32>& windowTypes()  const { return m_windowTypes; }

private:
    // Manager-Abhängigkeiten
    FlagManager*   m_flagMgr   = nullptr;
    TextManager*   m_textMgr   = nullptr;
    DefineManager* m_defineMgr = nullptr;
    LayoutManager* m_layoutMgr = nullptr;  // Zugriff auf Layout-/Fensterkontext

    // 🔹 Flags & Types (früher: LayoutManager::m_windowFlags etc.)
    QMap<QString, quint32> m_windowFlags;
    QMap<QString, quint32> m_controlFlags;
    QMap<QString, quint32> m_windowTypes;

    // 🔹 Regeln (window_flag_rules.json / control_flag_rules.json)
    mutable QJsonObject m_windowRules;
    mutable QJsonObject m_controlRules;
    mutable bool m_windowRulesLoaded  = false;
    mutable bool m_controlRulesLoaded = false;

    // 🔹 Cache: gültige Flags pro Typ
    QMap<QString, QSet<QString>> m_validFlagsByType;

    // 🔹 Analyse: verfügbare Properties pro Control-Typ
    QMap<QString, QSet<QString>> m_controlTypeProperties;

    // 🔹 Gesammelte unbekannte Bits pro Control-Typ
    QMap<QString, QMap<QString, QSet<QString>>> m_unknownControlBits;

    // ------------------------------------------------------
    // 🔹 Interne Helfer (aus altem LayoutManager)
    // ------------------------------------------------------
    void reloadWindowFlagRules();
    void reloadControlFlagRules();
    void rebuildValidFlagCache();
};
