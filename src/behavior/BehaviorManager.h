#pragma once

#include <QMap>
#include <QVariant>
#include <QJsonObject>
#include <QString>
#include <QFlags>
#include <memory>
#include <vector>

struct BehaviorInfo {
    QString category;
    QMap<QString, QVariant> attributes;
};

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

struct BaseBehavior
{
    QString category;
    ControlCapabilities capabilities;
    QMap<QString, QVariant> defaults;
};

class FlagManager;
class TextManager;
class DefineManager;
class LayoutManager;
class LayoutBackend;
struct ControlData;
struct WindowData;

class BehaviorManager
{
public:
    BehaviorManager(FlagManager* flagMgr,
                    TextManager* textMgr,
                    DefineManager* defineMgr,
                    LayoutManager* layoutMgr,
                    LayoutBackend* layoutBackend);

    // --- Flags laden ---
    void refreshFlagsFromFiles(const QString& wndFlagsPath = {},
                               const QString& ctrlFlagsPath = {});

    const QMap<QString, quint32>& windowFlags()  const { return m_windowFlags; }
    const QMap<QString, quint32>& controlFlags() const { return m_controlFlags; }

    QJsonObject windowFlagRules() const;
    QJsonObject controlFlagRules() const;

    // --- Flags interpretieren ---
    void updateWindowFlags(const std::shared_ptr<WindowData>& wnd) const;
    void updateControlFlags(const std::shared_ptr<ControlData>& ctrl) const;
    void applyWindowStyle(WindowData& wnd) const;

    // --- Validierung ---
    void validateWindowFlags(WindowData* wnd) const;
    void validateControlFlags(ControlData* ctrl) const;

    // --- Analyse (optional) ---
    void analyzeControlTypes(const std::vector<std::shared_ptr<WindowData>>& windows) const;
    void generateUnknownControls(const std::vector<std::shared_ptr<WindowData>>& windows) const;

    // ===========================================
    // Behavior API – finale öffentliche Funktionen
    // ===========================================
    BehaviorInfo resolveBehavior(const ControlData& ctrl) const;
    BehaviorInfo resolveBehavior(const WindowData& wnd) const;

private:
    // --- Manager ---
    FlagManager*    m_flagMgr   = nullptr;
    TextManager*    m_textMgr   = nullptr;
    DefineManager*  m_defineMgr = nullptr;
    LayoutManager*  m_layoutMgr = nullptr;
    LayoutBackend*  m_layoutBackend = nullptr;

    // --- interne Helfer ---
    QMap<QString, QVariant> resolveControlSemantic(const ControlData& ctrl) const;
    QMap<QString, QVariant> resolveWindowSemantic(const WindowData& wnd) const;

    QMap<QString, QVariant> deriveCombinedBehavior(
        const QString& normalizedType,
        const QMap<QString, QVariant>& semantic) const;

    QString normalizeType(const QString& type) const;

    // --- Flags ---
    QMap<QString, quint32> m_windowFlags;
    QMap<QString, quint32> m_controlFlags;

    // --- Rules ---
    mutable QJsonObject m_windowRules;
    mutable QJsonObject m_controlRules;
    mutable bool m_windowRulesLoaded  = false;
    mutable bool m_controlRulesLoaded = false;

    // --- BaseBehaviors ---
    QMap<QString, BaseBehavior> m_baseBehaviors;

    // --- Optionale BehaviorConfig (noch leer) ---
    mutable QJsonObject m_behaviorConfig;
    mutable bool m_behaviorConfigLoaded = false;

    // --- Initialisierung ---
    void initializeBaseBehaviors();
    void reloadWindowFlagRules() const;
    void reloadControlFlagRules() const;
    void reloadBehaviorConfig() const;
};
