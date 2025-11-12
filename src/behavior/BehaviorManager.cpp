#include "BehaviorManager.h"
#include "define/FlagManager.h"
#include "define/DefineManager.h"
#include "text/TextManager.h"
#include "layout/LayoutManager.h"
#include "layout/LayoutBackend.h"
#include "layout/model/WindowData.h"
#include "layout/model/ControlData.h"

#include <QJsonArray>
#include <QDebug>

Q_DECLARE_METATYPE(ControlCapabilities)

// ---------------------------------------------------------
// Konstruktor
// ---------------------------------------------------------
BehaviorManager::BehaviorManager(FlagManager*   flagMgr,
                                 TextManager*   textMgr,
                                 DefineManager* defineMgr,
                                 LayoutManager* layoutMgr,
                                 LayoutBackend* layoutBackend)
    : m_flagMgr(flagMgr)
    , m_textMgr(textMgr)
    , m_defineMgr(defineMgr)
    , m_layoutMgr(layoutMgr)
    , m_layoutBackend(layoutBackend)
{
    initializeBaseBehaviors();
}

// ---------------------------------------------------------
// Basisverhalten für alle relevanten WTYPE_* hartcodieren
// ---------------------------------------------------------
void BehaviorManager::initializeBaseBehaviors()
{
    m_baseBehaviors.clear();

    // --------------------------
    // Button
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "button";
        b.capabilities  = ControlCapability_CanClick
                         | ControlCapability_CanFocus;
        b.defaults["state"]        = "normal";
        b.defaults["defaultColor"] = QColor(255, 255, 255);
        b.defaults["textSupport"]  = true;
        m_baseBehaviors.insert("WTYPE_BUTTON", b);
    }

    // --------------------------
    // Static / Label
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "label";
        b.capabilities  = ControlCapability_None;
        b.defaults["textSupport"]  = true;
        b.defaults["textAlign"]    = "center";
        m_baseBehaviors.insert("WTYPE_STATIC", b);
    }

    // --------------------------
    // Textanzeige (mehrzeilig)
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "text";
        b.capabilities  = ControlCapability_CanScroll;
        b.defaults["textSupport"]  = true;
        b.defaults["multiline"]    = true;
        m_baseBehaviors.insert("WTYPE_TEXT", b);
    }

    // --------------------------
    // Editfeld
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "edit";
        b.capabilities  = ControlCapability_CanFocus
                         | ControlCapability_CanTextInput
                         | ControlCapability_CanScroll;
        b.defaults["textSupport"]  = true;
        b.defaults["multiline"]    = false;
        b.defaults["maxLength"]    = 128;
        m_baseBehaviors.insert("WTYPE_EDITCTRL", b);
    }

    // --------------------------
    // Listbox
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "listbox";
        b.capabilities  = ControlCapability_CanFocus
                         | ControlCapability_CanSelectItems
                         | ControlCapability_CanScroll;
        b.defaults["multiSelect"]  = false;
        m_baseBehaviors.insert("WTYPE_LISTBOX", b);
    }

    // --------------------------
    // Kombobox
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "combobox";
        b.capabilities  = ControlCapability_CanFocus
                         | ControlCapability_CanSelectItems
                         | ControlCapability_CanClick
                         | ControlCapability_CanScroll;
        b.defaults["textSupport"]  = true;  // editable combos möglich
        m_baseBehaviors.insert("WTYPE_COMBOBOX", b);
    }

    // --------------------------
    // TabControl
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "tab";
        b.capabilities  = ControlCapability_CanClick
                         | ControlCapability_CanFocus
                         | ControlCapability_IsContainer;
        b.defaults["hasTabs"]      = true;
        m_baseBehaviors.insert("WTYPE_TABCTRL", b);
    }

    // --------------------------
    // TreeControl
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "tree";
        b.capabilities  = ControlCapability_CanFocus
                         | ControlCapability_CanSelectItems
                         | ControlCapability_CanScroll
                         | ControlCapability_CanToggle;
        b.defaults["multiSelect"]  = false;
        m_baseBehaviors.insert("WTYPE_TREECTRL", b);
    }

    // --------------------------
    // GroupBox
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "groupbox";
        b.capabilities  = ControlCapability_IsContainer;
        b.defaults["textSupport"]  = true;
        m_baseBehaviors.insert("WTYPE_GROUPBOX", b);
    }

    // --------------------------
    // Custom
    // --------------------------
    {
        BaseBehavior b;
        b.category      = "custom";
        b.capabilities  = ControlCapability_CustomBehavior;
        m_baseBehaviors.insert("WTYPE_CUSTOM", b);
    }
}

// ---------------------------------------------------------
// Flags aus JSON laden (Fenster / Controls)
// ---------------------------------------------------------
void BehaviorManager::refreshFlagsFromFiles(const QString& wndFlagsPath,
                                            const QString& ctrlFlagsPath)
{
    Q_UNUSED(wndFlagsPath)
    Q_UNUSED(ctrlFlagsPath)

    if (!m_layoutBackend) {
        qWarning() << "[BehaviorManager] Kein LayoutBackend – Flags können nicht geladen werden.";
        m_windowFlags.clear();
        m_controlFlags.clear();
        return;
    }

    const QJsonObject winObj  = m_layoutBackend->loadWindowFlags();
    const QJsonObject ctrlObj = m_layoutBackend->loadControlFlags();

    m_windowFlags.clear();
    m_controlFlags.clear();

    m_windowRulesLoaded  = false;
    m_controlRulesLoaded = false;
    m_windowRules = QJsonObject{};
    m_controlRules = QJsonObject{};

    // 🪟 Window-Flags (High-Word)
    for (auto it = winObj.constBegin(); it != winObj.constEnd(); ++it)
    {
        QString val = it.value().toString().trimmed().toUpper();
        if (val.startsWith("0X")) val.remove(0, 2);
        if (val.endsWith("L"))   val.chop(1);

        bool ok = false;
        quint32 raw = val.toUInt(&ok, 16);
        if (ok)
            m_windowFlags[it.key()] = (raw << 16);
        else
            qWarning().noquote()
                << "[BehaviorManager] Ungültiger Window-Flag-Wert:"
                << it.key() << "=" << it.value().toVariant().toString();
    }

    // 🔹 Control-Flags (Low-Word)
    for (auto it = ctrlObj.constBegin(); it != ctrlObj.constEnd(); ++it)
    {
        QString val = it.value().toString().trimmed().toUpper();
        if (val.startsWith("0X")) val.remove(0, 2);
        if (val.endsWith("L"))   val.chop(1);

        bool ok = false;
        quint32 raw = val.toUInt(&ok, 16);
        if (ok)
            m_controlFlags[it.key()] = raw;
        else
            qWarning().noquote()
                << "[BehaviorManager] Ungültiger Control-Flag-Wert:"
                << it.key() << "=" << it.value().toString();
    }

    qInfo() << "[BehaviorManager] Flags geladen:"
            << "windows =" << m_windowFlags.size()
            << "controls =" << m_controlFlags.size();
}

// ---------------------------------------------------------
// Flag-Regeln lazy laden
// ---------------------------------------------------------
void BehaviorManager::reloadWindowFlagRules() const
{
    if (!m_layoutBackend) {
        qWarning() << "[BehaviorManager] Kein LayoutBackend – window_flag_rules.json kann nicht geladen werden.";
        m_windowRules = QJsonObject{};
        m_windowRulesLoaded = true;
        return;
    }

    m_windowRules = m_layoutBackend->loadWindowFlagRules();
    m_windowRulesLoaded = true;
}

void BehaviorManager::reloadControlFlagRules() const
{
    if (!m_layoutBackend) {
        qWarning() << "[BehaviorManager] Kein LayoutBackend – control_flag_rules.json kann nicht geladen werden.";
        m_controlRules = QJsonObject{};
        m_controlRulesLoaded = true;
        return;
    }

    m_controlRules = m_layoutBackend->loadControlFlagRules();
    m_controlRulesLoaded = true;
}

QJsonObject BehaviorManager::windowFlagRules() const
{
    if (!m_windowRulesLoaded)
        const_cast<BehaviorManager*>(this)->reloadWindowFlagRules();
    return m_windowRules;
}

QJsonObject BehaviorManager::controlFlagRules() const
{
    if (!m_controlRulesLoaded)
        const_cast<BehaviorManager*>(this)->reloadControlFlagRules();
    return m_controlRules;
}

// ---------------------------------------------------------
// Behavior-Konfiguration aus Datei (später erweiterbar)
// ---------------------------------------------------------
void BehaviorManager::reloadBehaviorConfig() const
{
    // Aktuell noch kein Backend-Call – Platzhalter.
    // Später: m_behaviorConfig = m_layoutBackend->loadBehaviorConfig();
    m_behaviorConfig = QJsonObject{};
    m_behaviorConfigLoaded = true;
}

// ---------------------------------------------------------
// Masken → resolvedMask aktualisieren
// ---------------------------------------------------------
void BehaviorManager::updateWindowFlags(const std::shared_ptr<WindowData>& wnd) const
{
    if (!wnd)
        return;

    wnd->resolvedMask.clear();
    for (auto it = m_windowFlags.constBegin(); it != m_windowFlags.constEnd(); ++it) {
        if (wnd->flagsMask & it.value())
            wnd->resolvedMask << it.key();
    }
}

void BehaviorManager::updateControlFlags(const std::shared_ptr<ControlData>& ctrl) const
{
    if (!ctrl)
        return;

    ctrl->resolvedMask.clear();
    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it) {
        if (ctrl->flagsMask & it.value())
            ctrl->resolvedMask << it.key();
    }
}

void BehaviorManager::applyWindowStyle(WindowData& wnd) const
{
    wnd.resolvedMask.clear();

    const quint32 style = wnd.flagsMask;

    auto has = [&](const QString& key) -> bool {
        return style & m_windowFlags.value(key, 0);
    };

    //
    // 🧩 Basis-Fensterverhalten
    //
    if (has("WBS_MOVE"))       wnd.resolvedMask.append("movable");
    if (has("WBS_MODAL"))      wnd.resolvedMask.append("modal");
    if (has("WBS_CHILD"))      wnd.resolvedMask.append("is_child");
    if (has("WBS_TOPMOST"))    wnd.resolvedMask.append("always_on_top");

    //
    // 🪟 Rahmen & Caption
    //
    if (has("WBS_THICKFRAME") || has("WBS_RESIZEABLE"))
        wnd.resolvedMask.append("resizable");

    if (has("WBS_CAPTION"))
        wnd.resolvedMask.append("has_caption");
    else
        wnd.resolvedMask.append("no_caption");

    if (has("WBS_NOFRAME"))
        wnd.resolvedMask.append("no_frame");
    else
        wnd.resolvedMask.append("has_frame");

    //
    // 🎛️ Titelbuttons
    // Hier interpretieren wir das 0x80-Bit (NOCLOSE/NOCENTER) kontextabhängig
    //
    static const QStringList hudWindows = {
        "APP_MINIMAP",
        "APP_HP_GAUGE",
        "APP_QUICK_SLOT",
        "APP_TARGET_INFO",
        "APP_CHAT",
        "APP_PLAYER_INFO",
        "APP_BUFF",
        "APP_ACTION_SLOT"
    };

    const bool isHudWindow =
        hudWindows.contains(wnd.name, Qt::CaseInsensitive);

    const bool hasNoCloseFlag  = has("WBS_NOCLOSE");
    const bool hasNoCenterFlag = has("WBS_NOCENTER");

    bool hideCloseButton = false;

    // 🧠 Regel:
    // - HUDs → NOCENTER aktiv, Close irrelevant
    // - normale Fenster → NOCLOSE aktiv, beeinflusst Buttonanzeige
    if (isHudWindow) {
        if (hasNoCenterFlag)
            wnd.resolvedMask.append("no_center");
        hideCloseButton = true;
    } else {
        hideCloseButton = hasNoCloseFlag;
    }

    // Close-Button nur anzeigen, wenn erlaubt
    if (!hideCloseButton)
        wnd.resolvedMask.append("has_close");

    //
    // 🧭 Weitere Standard-Buttons
    //
    if (has("WBS_HELP"))        wnd.resolvedMask.append("has_help");
    if (has("WBS_PIN"))         wnd.resolvedMask.append("has_pin");
    if (has("WBS_VIEW"))        wnd.resolvedMask.append("has_view");
    if (has("WBS_EXTENSION"))   wnd.resolvedMask.append("has_extension");
    if (has("WBS_MINIMIZEBOX")) wnd.resolvedMask.append("has_minimize");
    if (has("WBS_MAXIMIZEBOX")) wnd.resolvedMask.append("has_maximize");

    //
    // 🧩 Sichtbarkeit & Fallback
    //
    if (has("WBS_VISIBLE"))     wnd.resolvedMask.append("visible");
    if (!wnd.resolvedMask.contains("has_frame") &&
        !wnd.resolvedMask.contains("no_frame"))
        wnd.resolvedMask.append("default_frame");

    //
    // 🧾 Debug-Ausgabe
    //
    qDebug().noquote()
        << QString("[BehaviorManager] applyWindowStyle → %1 (0x%2)")
               .arg(wnd.name)
               .arg(style, 0, 16)
        << "\n → resolvedMask:" << wnd.resolvedMask;
}
// ---------------------------------------------------------
// Validierung – aktuell sehr einfach, kann später ausgebaut werden
// ---------------------------------------------------------
void BehaviorManager::validateWindowFlags(WindowData* wnd) const
{
    if (!wnd)
        return;

    // Beispiel: ungültige Bits löschen
    quint32 knownMask = 0;
    for (auto it = m_windowFlags.constBegin(); it != m_windowFlags.constEnd(); ++it)
        knownMask |= it.value();

    if ((wnd->flagsMask & ~knownMask) != 0) {
        qWarning().noquote()
        << "[BehaviorManager] Window" << wnd->name
        << "enthält unbekannte Flagbits:"
        << QString("0x%1").arg(wnd->flagsMask & ~knownMask, 0, 16);
    }

    // resolvedMask im Anschluss neu aufbauen
    std::shared_ptr<WindowData> shared; // nur für updateWindowFlags-API
    // Hinweis: In deinem Code nutzt du normalerweise shared_ptr,
    // hier ruft LayoutManager::processLayout() die Validierung
    // mit raw-Pointern. Wenn du willst, kannst du updateWindowFlags
    // an raw-Pointer overloaden. Fürs Erste lassen wir es hier minimal.
}

void BehaviorManager::validateControlFlags(ControlData* ctrl) const
{
    if (!ctrl)
        return;

    quint32 knownMask = 0;
    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
        knownMask |= it.value();

    if ((ctrl->flagsMask & ~knownMask) != 0) {
        qWarning().noquote()
        << "[BehaviorManager] Control" << ctrl->id
        << "enthält unbekannte Flagbits:"
        << QString("0x%1").arg(ctrl->flagsMask & ~knownMask, 0, 16);
    }
}

void BehaviorManager::analyzeControlTypes(const std::vector<std::shared_ptr<WindowData>>& windows) const
{
    Q_UNUSED(windows);
    // Hier könntest du z.B. Statistiken sammeln,
    // welche Controls es mit welchen Typen/Flags gibt.
}

void BehaviorManager::generateUnknownControls(const std::vector<std::shared_ptr<WindowData>>& windows) const
{
    Q_UNUSED(windows);
    // Optional: "Unknown" Controls in Listen sammeln,
    // um sie im Editor speziell darzustellen.
}

// ---------------------------------------------------------
// Behavior-API – BaseBehavior + optionale Config
// ---------------------------------------------------------
BehaviorInfo BehaviorManager::resolveBehavior(const ControlData& ctrl) const
{
    BehaviorInfo info;

    // 1️⃣ BaseBehavior
    BaseBehavior base;
    bool hasBase = m_baseBehaviors.contains(ctrl.type);
    if (hasBase) {
        base = m_baseBehaviors.value(ctrl.type);
        info.category = base.category;
        for (auto it = base.defaults.constBegin(); it != base.defaults.constEnd(); ++it)
            info.attributes.insert(it.key(), it.value());
    } else {
        info.category = "unknown";
    }

    // Capabilities in eine einfache Liste schreiben (gut für Debug/UI)
    QStringList capsList;
    if (hasBase) {
        ControlCapabilities caps = base.capabilities;
        if (caps.testFlag(ControlCapability_CanClick))       capsList << "CanClick";
        if (caps.testFlag(ControlCapability_CanToggle))      capsList << "CanToggle";
        if (caps.testFlag(ControlCapability_CanFocus))       capsList << "CanFocus";
        if (caps.testFlag(ControlCapability_CanTextInput))   capsList << "CanTextInput";
        if (caps.testFlag(ControlCapability_CanSelectItems)) capsList << "CanSelectItems";
        if (caps.testFlag(ControlCapability_CanScroll))      capsList << "CanScroll";
        if (caps.testFlag(ControlCapability_IsContainer))    capsList << "IsContainer";
        if (caps.testFlag(ControlCapability_HasTooltip))     capsList << "HasTooltip";
        if (caps.testFlag(ControlCapability_CustomBehavior)) capsList << "CustomBehavior";
    }
    info.attributes["capabilities"] = capsList;

    // 2️⃣ Behavior-Config aus Datei (noch leer, aber erweiterbar)
    if (!m_behaviorConfigLoaded)
        const_cast<BehaviorManager*>(this)->reloadBehaviorConfig();

    if (m_behaviorConfig.contains(ctrl.type)) {
        QJsonObject obj = m_behaviorConfig.value(ctrl.type).toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it)
            info.attributes[it.key()] = it.value().toVariant();
    }

    // 3️⃣ Laufzeitabhängige Informationen / Flags
    info.attributes["id"]      = ctrl.id;
    info.attributes["type"]    = ctrl.type;
    info.attributes["color"]   = ctrl.color;
    info.attributes["enabled"] = true;   // später aus Flags ableiten
    info.attributes["visible"] = true;   // ebenfalls aus Flags ableitbar

    return info;
}

// ---------------------------------------------------------
// Defaults anwenden (optional)
// ---------------------------------------------------------
void BehaviorManager::applyBehavior(ControlData& ctrl) const
{
    Q_UNUSED(ctrl);
    // Hier könntest du später z.B. defaultColor, TextAlign, etc.
    // tatsächlich in ControlData zurückschreiben.
}
