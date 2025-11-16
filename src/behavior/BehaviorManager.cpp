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

    auto add = [&](const QString& behaviorType,
                   const QString& category,
                   ControlCapabilities caps,
                   const QMap<QString, QVariant>& defs = {})
    {
        BaseBehavior b;
        b.category     = category;
        b.capabilities = caps;
        b.defaults     = defs;
        m_baseBehaviors.insert(behaviorType, b);
    };

    //
    // 🔹 Basis / Fallback
    //
    add("base",
        "base",
        ControlCapability_None
        );

    //
    // 🔹 Label / Static / Text / Icon / Html usw.
    //
    add("label",
        "label",
        ControlCapability_None,
        {
            {"textSupport", true},
            {"textAlign",   "left"},
            {"imageMode",   "none"}
        }
        );

    //
    // 🔹 Button (normaler Klick-Button)
    //
    add("button",
        "button",
        ControlCapability_CanClick |
            ControlCapability_CanFocus |
            ControlCapability_HasTooltip,
        {
            {"textSupport",  true},
            {"defaultState", "normal"}
        }
        );

    //
    // 🔹 Edit / TextInput
    //
    add("edit",
        "textinput",
        ControlCapability_CanTextInput |
            ControlCapability_CanFocus |
            ControlCapability_CanScroll,
        {
            {"textSupport", true},
            {"multiline",   false},
            {"password",    false},
            {"readonly",    false},
            {"textAlign",   "left"},
            {"maxLength",   128}
        }
        );

    //
    // 🔹 Listbox / Listview
    //
    add("listbox",
        "list",
        ControlCapability_CanSelectItems |
            ControlCapability_CanScroll |
            ControlCapability_CanFocus,
        {
            {"multiSelect", false},
            {"sorted",      false}
        }
        );

    //
    // 🔹 Combobox
    //
    add("combobox",
        "combobox",
        ControlCapability_CanSelectItems |
            ControlCapability_CanScroll |
            ControlCapability_CanFocus |
            ControlCapability_CanClick,
        {
            {"editable",     false},
            {"dropDownSize", 8}
        }
        );

    //
    // 🔹 Tree / ViewTree
    //
    add("tree",
        "tree",
        ControlCapability_CanSelectItems |
            ControlCapability_CanScroll |
            ControlCapability_CanFocus |
            ControlCapability_IsContainer |
            ControlCapability_CanToggle,
        {
            {"multiSelect", false}
        }
        );

    //
    // 🔹 TabControl
    //
    add("tab",
        "tab",
        ControlCapability_CanSelectItems |
            ControlCapability_CanClick |
            ControlCapability_CanFocus |
            ControlCapability_IsContainer,
        {
            {"hasTabs", true}
        }
        );

    //
    // 🔹 Scrollbar / Slider
    //
    add("scrollbar",
        "scrollbar",
        ControlCapability_CanScroll |
            ControlCapability_CanFocus,
        {
            {"orientation", "vertical"}   // wird später durch Flags überschrieben
        }
        );

    //
    // 🔹 Progress / Gauge
    //
    add("progress",
        "progress",
        ControlCapability_None,
        {
            {"min",   0},
            {"max",   100},
            {"value", 0}
        }
        );

    //
    // 🔹 Groupbox (Container mit Rahmen + Text)
    //
    add("groupbox",
        "groupbox",
        ControlCapability_IsContainer,
        {
            {"textSupport", true}
        }
        );

    //
    // 🔹 Custom (alles was die Engine speziell macht)
    //
    add("custom",
        "custom",
        ControlCapability_CustomBehavior
        );

    qInfo() << "[BehaviorManager] BaseBehaviors initialisiert:" << m_baseBehaviors.size();
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
            m_windowFlags[it.key()] = raw;
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

    // Low-Word = ControlFlags
    const quint32 style = ctrl->flagsMask & 0xFFFF;

    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
    {
        if (style & it.value())
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
    // qDebug().noquote()
    //     << QString("[BehaviorManager] applyWindowStyle → %1 (0x%2)")
    //            .arg(wnd.name)
    //            .arg(style, 0, 16)
    //     << "\n → resolvedMask:" << wnd.resolvedMask;
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

    //
    // ----------------------------
    // 1) KNOWN MASKS erzeugen
    // ----------------------------
    //
    quint32 knownControlMask = 0;
    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
        knownControlMask |= it.value();        // BS_*, ES_*, LBS_*, SS_*

    quint32 knownWindowMask = 0;
    for (auto it = m_windowFlags.constBegin(); it != m_windowFlags.constEnd(); ++it)
        knownWindowMask |= it.value();         // WBS_* (High+Mid Bits)

    //
    // ----------------------------
    // 2) Bits extrahieren
    // ----------------------------
    //
    const quint32 mask = ctrl->flagsMask;

    const quint32 lowBits  =  mask        & 0x0000FFFF;  // Control styles
    const quint32 midBits  = (mask >> 16) & 0x000000FF;  // WBS group
    const quint32 highBits = (mask >> 24) & 0x000000FF;  // WBS group

    //
    // ----------------------------
    // 3) UNKNOWN-BITS berechnen
    // ----------------------------
    //

    // LOW word must match ControlFlags
    quint32 lowUnknown = lowBits & ~knownControlMask;

    // MID & HIGH must match WindowFlags (WBS)
    quint32 midUnknown  = (mask & 0x00FF0000) & ~knownWindowMask;
    quint32 highUnknown = (mask & 0xFF000000) & ~knownWindowMask;

    //
    // ----------------------------
    // 4) Ausgabe NUR wenn nötig
    // ----------------------------
    //
    if (lowUnknown != 0)
    {
        qWarning().noquote()
        << "[BehaviorManager] Control" << ctrl->id
        << "enthält unbekannte LOW-Flags:"
        << QString("0x%1").arg(lowUnknown, 0, 16);
    }

    if (midUnknown != 0)
    {
        qWarning().noquote()
        << "[BehaviorManager] Control" << ctrl->id
        << "enthält unbekannte MID-Flags (Bits 16–23):"
        << QString("0x%1").arg(midUnknown >> 16, 0, 16);
    }

    if (highUnknown != 0)
    {
        qWarning().noquote()
        << "[BehaviorManager] Control" << ctrl->id
        << "enthält unbekannte HIGH-Flags (Bits 24–31):"
        << QString("0x%1").arg(highUnknown >> 24, 0, 16);
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
    // 🔥 Engine → Behavior Typ normalisieren
    QString normalized = normalizeType(ctrl.type);

    BehaviorInfo info;

    //
    // =========================================================
    // 1) BaseBehavior
    // =========================================================
    //
    BaseBehavior base;
    bool hasBase = m_baseBehaviors.contains(normalized);

    if (hasBase) {
        base = m_baseBehaviors.value(normalized);

        // Kategorie übernehmen
        info.category = base.category;

        // Default-Werte übernehmen
        for (auto it = base.defaults.constBegin(); it != base.defaults.constEnd(); ++it)
            info.attributes.insert(it.key(), it.value());
    }
    else {
        info.category = "unknown";
    }

    //
    // =========================================================
    // 1.5) Capabilities übernehmen
    // =========================================================
    //
    if (hasBase)
    {
        QStringList capsList;
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

        info.attributes["capabilities"] = capsList;
    }

    //
    // =========================================================
    // 2) BehaviorConfig.json (falls später vorhanden)
    // =========================================================
    //
    if (!m_behaviorConfigLoaded)
        const_cast<BehaviorManager*>(this)->reloadBehaviorConfig();

    if (m_behaviorConfig.contains(normalized)) {
        const QJsonObject obj = m_behaviorConfig.value(normalized).toObject();
        for (auto it = obj.begin(); it != obj.end(); ++it)
            info.attributes[it.key()] = it.value().toVariant();
    }

    //
    // =========================================================
    // 3) Flag-Semantik => Rohinterpretation der Flags
    // =========================================================
    //
    QMap<QString, QVariant> semantic = resolveControlSemantic(ctrl);

    // Semantik direkt beim Mergen berücksichtigen
    for (auto it = semantic.begin(); it != semantic.end(); ++it)
        info.attributes[it.key()] = it.value();

    //
    // =========================================================
    // 4) Combined Behavior (ABHÄNGIG VON TYP + SEMANTIK)
    // =========================================================

    QString engineType = ctrl.type.trimmed().toUpper();
    QMap<QString, QVariant> combined = deriveCombinedBehavior(engineType, semantic);

    for (auto it = combined.begin(); it != combined.end(); ++it)
        info.attributes[it.key()] = it.value();

    if (combined.contains("category"))
        info.category = combined["category"].toString();

    //
    // =========================================================
    // 5) Runtime-Attribute (Editor)
    // =========================================================
    //
    info.attributes["id"]      = ctrl.id;
    info.attributes["type"]    = normalized;
    info.attributes["color"]   = ctrl.color;
    info.attributes["enabled"] = !info.attributes.value("enabled", true).toBool() ? false : true;
    info.attributes["visible"] = info.attributes.value("visible", true).toBool();

    return info;
}

BehaviorInfo BehaviorManager::resolveBehavior(const WindowData& wnd) const
{
    BehaviorInfo info;
    info.category = "window";

    // 1) Fensterstil zu resolvedMask
    WindowData temp = wnd;      // copy
    applyWindowStyle(temp);     // schreibt resolvedMask

    QMap<QString, QVariant> attrs;

    for (const QString& f : temp.resolvedMask)
        attrs[f] = true;

    // 2) Semantik
    QMap<QString, QVariant> semantic = resolveWindowSemantic(wnd);
    for (auto it = semantic.begin(); it != semantic.end(); ++it)
        attrs[it.key()] = it.value();

    // 3) Runtime
    attrs["name"]    = wnd.name;
    attrs["enabled"] = attrs.value("enabled", true);
    attrs["visible"] = attrs.value("visible", true);

    info.attributes = attrs;
    return info;
}

// ============================================================
//  Phase 1: ControlFlag-Semantik auflösen
// ============================================================
QMap<QString, QVariant> BehaviorManager::resolveControlSemantic(const ControlData& ctrl) const
{
    QMap<QString, QVariant> out;

    const quint32 flags = ctrl.lowFlags;

    // Helper: test if flag is set
    auto has = [&](const QString& key) -> bool {
        return (flags & m_controlFlags.value(key, 0)) != 0;
    };

    //
    // ============================================================
    // BUTTON-FLAGS (BS_*)
    // ============================================================
    //
    if (has("BS_CHECKBOX") || has("BS_AUTOCHECKBOX")) {
        out["role"]   = "checkbox";
        out["toggle"] = true;
    }

    if (has("BS_3STATE") || has("BS_AUTO3STATE")) {
        out["role"]    = "checkbox";
        out["toggle"]  = true;
        out["triState"] = true;
    }

    if (has("BS_RADIOBUTTON") || has("BS_AUTORADIOBUTTON")) {
        out["role"]   = "radiobutton";
        out["toggle"] = true;
    }

    if (has("BS_DEFPUSHBUTTON")) {
        out["defaultButton"] = true;
    }

    // Button-Textausrichtung
    if (has("BS_LEFT"))     out["textAlign"] = "left";
    if (has("BS_RIGHT"))    out["textAlign"] = "right";
    if (has("BS_TOP"))      out["textAlignV"] = "top";
    if (has("BS_BOTTOM"))   out["textAlignV"] = "bottom";
    if (has("BS_VCENTER"))  out["textAlignV"] = "center";


    //
    // ============================================================
    // EDIT-FELDER (ES_*/EBS_*)
    // ============================================================
    //
    if (has("ES_PASSWORD"))   out["password"] = true;
    if (has("ES_READONLY"))   out["readonly"] = true;
    if (has("ES_MULTILINE"))  out["multiline"] = true;

    // Textausrichtung (horizontal)
    if (has("ES_CENTER"))      out["textAlign"] = "center";
    else if (has("ES_RIGHT"))  out["textAlign"] = "right";
    else                       out["textAlign"] = "left";

    // Verhalten
    if (has("ES_AUTOHSCROLL")) out["autoScrollX"] = true;
    if (has("ES_AUTOVSCROLL")) out["autoScrollY"] = true;
    if (has("ES_NOHIDESEL"))   out["noHideSelection"] = true;
    if (has("ES_OEMCONVERT"))  out["oemConvert"] = true;
    if (has("ES_NUMBER"))      out["numeric"] = true;
    if (has("ES_WANTRETURN"))  out["acceptReturn"] = true;


    //
    // ============================================================
    // LISTBOX-FLAGS (LBS_*)
    // ============================================================
    //
    if (has("LBS_MULTIPLESEL"))    out["multiSelect"] = true;
    if (has("LBS_EXTENDEDSEL"))    out["extendedSelect"] = true;
    if (has("LBS_SORT"))           out["sorted"] = true;
    if (has("LBS_USETABSTOPS"))    out["tabStops"] = true;
    if (has("LBS_OWNERDRAWFIXED")) out["ownerDraw"] = "fixed";
    if (has("LBS_OWNERDRAWVARIABLE")) out["ownerDraw"] = "variable";
    if (has("LBS_HASSTRINGS"))     out["hasStrings"] = true;
    if (has("LBS_NOINTEGRALHEIGHT")) out["noIntegralHeight"] = true;
    if (has("LBS_DISABLENOSCROLL")) out["disableNoScroll"] = true;
    if (has("LBS_NOTIFY"))         out["notify"] = true;
    if (has("LBS_MULTICOLUMN"))    out["multiColumn"] = true;
    if (has("LBS_WANTKEYBOARDINPUT")) out["wantKeyboard"] = true;


    //
    // ============================================================
    // STATIC-CONTROLS (SS_*)
    // ============================================================
    //
    if (has("SS_CENTER")) out["textAlign"] = "center";
    if (has("SS_RIGHT"))  out["textAlign"] = "right";

    if (has("SS_NOTIFY")) out["notify"] = true;
    if (has("SS_BITMAP")) out["imageMode"] = "bitmap";
    if (has("SS_ICON"))   out["imageMode"] = "icon";


    //
    // ============================================================
    // SCROLLBARS (SBS_*)
    // ============================================================
    //
    if (has("SBS_VERT")) out["orientation"] = "vertical";
    if (has("SBS_HORZ")) out["orientation"] = "horizontal";


    //
    // ============================================================
    // WINDOW-STYLE (general WS_*)
    // ============================================================
    //
    if (has("WS_DISABLED"))
        out["enabled"] = false;
    else
        out["enabled"] = true;

    if (has("WS_VISIBLE"))
        out["visible"] = true;


    return out;
}

QMap<QString, QVariant> BehaviorManager::resolveWindowSemantic(const WindowData& wnd) const
{
    QMap<QString, QVariant> out;

    const quint32 flags = wnd.flagsMask;  // High word = WindowFlags

    // Helper to test window flag status
    auto has = [&](const QString& key) -> bool {
        return (flags & m_windowFlags.value(key, 0)) != 0;
    };

    //
    // ============================================================
    // BASIC WINDOW PROPERTIES
    // ============================================================
    //
    if (has("WBS_VISIBLE"))
        out["visible"] = true;
    else
        out["visible"] = false;

    if (has("WBS_DISABLED"))
        out["enabled"] = false;
    else
        out["enabled"] = true;

    if (has("WBS_CHILD"))
        out["isChild"] = true;

    if (has("WBS_MODAL"))
        out["modal"] = true;

    if (has("WBS_TOPMOST"))
        out["topMost"] = true;


    //
    // ============================================================
    // CAPTION / TITLE / FRAME
    // ============================================================
    //
    if (has("WBS_CAPTION"))
        out["hasCaption"] = true;
    else
        out["hasCaption"] = false;

    if (has("WBS_TITLE"))
        out["hasTitle"] = true;

    if (has("WBS_SYSMENU"))
        out["hasSysMenu"] = true;

    if (has("WBS_FRAME"))
        out["hasFrame"] = true;

    if (has("WBS_BORDER"))
        out["hasBorder"] = true;

    if (has("WBS_TOOLWINDOW"))
        out["toolWindow"] = true;


    //
    // ============================================================
    // FRAME MODES
    // ============================================================
    //

    if (has("WBS_THICKFRAME"))
        out["resizable"] = true;

    if (has("WBS_SIZE"))
        out["sizeable"] = true;

    if (has("WBS_NOFRAME"))
        out["noFrame"] = true;

    if (has("WBS_NODRAWFRAME"))
        out["noDrawFrame"] = true;


    //
    // ============================================================
    // SCROLLBARS
    // ============================================================
    //
    if (has("WBS_HSCROLL"))
        out["hScroll"] = true;

    if (has("WBS_VSCROLL"))
        out["vScroll"] = true;


    //
    // ============================================================
    // SPECIAL BEHAVIOR FLAGS
    // ============================================================
    //
    if (has("WBS_DOCKING"))
        out["docking"] = true;

    if (has("WBS_MOVE"))
        out["movable"] = true;

    if (has("WBS_MINIMIZEBOX"))
        out["hasMinimizeBox"] = true;

    if (has("WBS_MAXIMIZEBOX"))
        out["hasMaximizeBox"] = true;

    if (has("WBS_HELP"))
        out["hasHelpButton"] = true;

    if (has("WBS_PIN"))
        out["hasPinButton"] = true;

    if (has("WBS_VIEW"))
        out["hasViewButton"] = true;

    if (has("WBS_EXTENSION"))
        out["hasExtensionButton"] = true;


    //
    // ============================================================
    // CLOSE BUTTON HANDLING (Matches your applyWindowStyle logic)
    // ============================================================
    //

    bool hasNoClose = has("WBS_NOCLOSE");
    bool hasNoCenter = has("WBS_NOCENTER");

    bool hideClose = false;

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

    bool isHud = hudWindows.contains(wnd.name, Qt::CaseInsensitive);

    if (isHud) {
        if (hasNoCenter)
            out["noCenter"] = true;

        hideClose = true;
    } else {
        hideClose = hasNoClose;
    }

    out["hasCloseButton"] = (!hideClose);

    return out;
}

QMap<QString, QVariant> BehaviorManager::deriveCombinedBehavior(
    const QString& t,
    const QMap<QString, QVariant>& s) const
{
    QMap<QString, QVariant> out;

    //
    // =====================================================
    // BUTTON / CHECKBOX / RADIO
    // =====================================================
    //
    if (t == "WTYPE_BUTTON")
    {
        if (s.contains("triState"))
            out["category"] = "tristate_checkbox";
        else if (s.contains("role") && s["role"] == "checkbox")
            out["category"] = "checkbox";
        else if (s.contains("role") && s["role"] == "radiobutton")
            out["category"] = "radiobutton";
        else
            out["category"] = "button";

        if (s.contains("toggle"))
            out["toggle"] = s["toggle"];

        if (s.contains("defaultButton"))
            out["defaultButton"] = true;

        if (s.contains("textAlign"))
            out["textAlign"] = s["textAlign"];
    }

    //
    // =====================================================
    // EDIT
    // =====================================================
    //
    if (t == "WTYPE_EDIT")
    {
        out["category"] = "edit";

        if (s.contains("password"))
            out["password"] = true;

        if (s.contains("multiline"))
            out["multiline"] = s["multiline"];

        if (s.contains("readonly"))
            out["readonly"] = true;

        if (s.contains("textAlign"))
            out["textAlign"] = s["textAlign"];
    }

    //
    // =====================================================
    // LISTBOX
    // =====================================================
    //
    if (t == "WTYPE_LISTBOX")
    {
        out["category"] = "listbox";

        if (s.contains("multiSelect"))
            out["multiSelect"] = s["multiSelect"];

        if (s.contains("extendedSelect"))
            out["extendedSelect"] = s["extendedSelect"];

        if (s.contains("ownerDraw"))
            out["ownerDraw"] = s["ownerDraw"];
    }

    //
    // =====================================================
    // STATIC → Label / GroupBox / Image
    // =====================================================
    //
    if (t == "WTYPE_STATIC")
    {
        // GroupBox
        if (s.contains("groupbox") || s.contains("WSS_GROUPBOX")) {
            out["category"] = "groupbox";
            out["isContainer"] = true;
        }
        // Images
        else if (s.contains("imageMode")) {
            out["category"] = "image";
            out["imageMode"] = s["imageMode"];
        }
        // Normal Label
        else {
            out["category"] = "label";
        }

        if (s.contains("textAlign"))
            out["textAlign"] = s["textAlign"];
    }

    //
    // =====================================================
    // SCROLLBAR
    // =====================================================
    //
    if (t == "WTYPE_SCROLLBAR")
    {
        out["category"] = "scrollbar";

        if (s.contains("orientation"))
            out["orientation"] = s["orientation"];
        else
            out["orientation"] = "vertical"; // Default
    }

    //
    // =====================================================
    // TREE
    // =====================================================
    //
    if (t == "WTYPE_TREECTRL")
    {
        out["category"] = "tree";

        if (s.contains("multiSelect"))
            out["multiSelect"] = s["multiSelect"];
    }

    //
    // =====================================================
    // TAB
    // =====================================================
    //
    if (t == "WTYPE_TABCTRL")
    {
        out["category"] = "tab";
        out["hasTabs"] = true;
    }

    //
    // Fallback
    //
    if (!out.contains("category"))
        out["category"] = t.toLower();

    return out;
}


QString BehaviorManager::normalizeType(const QString& type) const
{
    QString t = type.trimmed().toUpper();

    // --------------------------------------
    // EXAKTE Engine-Typen (aus ResData.inc)
    // --------------------------------------
    static const QMap<QString, QString> engineToBehavior = {

    // --- Standard UI ---
    { "WTYPE_NONE",        "base" },
        { "WTYPE_BASE",        "base" },
        { "WTYPE_STATIC",      "label" },
        { "WTYPE_BUTTON",      "button" },
        { "WTYPE_EDIT",        "edit" },
        { "WTYPE_SCROLLBAR",   "scrollbar" },
        { "WTYPE_LISTBOX",     "listbox" },
        { "WTYPE_COMBOBOX",    "combobox" },
        { "WTYPE_TREECTRL",    "tree" },
        { "WTYPE_TABCTRL",     "tab" },
        { "WTYPE_CUSTOM",      "custom" },

        // --- Editor-intern / alternative Namen ---
        { "WTYPE_EDITCTRL",    "edit" },
        { "WTYPE_LISTCTRL",    "listbox" },
        { "WTYPE_GROUPBOX",    "groupbox" },
        { "WTYPE_TABPAGE",     "label" },       // Static page
        { "WTYPE_ITEMICON",    "custom" },

        // --- Zusätzliche Engine-Typen aus resdata.inc ---
        { "WTYPE_ICON",        "label" },
        { "WTYPE_TEXT",        "label" },
        { "WTYPE_PROGRESS",    "progress" },
        { "WTYPE_GAUGE",       "progress" },
        { "WTYPE_GAUGEEXT",    "progress" },
        { "WTYPE_HTML",        "label" },
        { "WTYPE_RICHTEXT",    "label" },
        { "WTYPE_SCRIPT",      "custom" },
        { "WTYPE_ANIMATE",     "custom" },
        { "WTYPE_LISTVIEW",    "listbox" },
        { "WTYPE_SLIDER",      "scrollbar" },
        { "WTYPE_MESH",        "custom" },
        { "WTYPE_VIEWTREE",    "tree" },
        { "WTYPE_DIALOGCTRL",  "custom" }
};

if (engineToBehavior.contains(t))
    return engineToBehavior[t];

// Fallback: Unbekannt → als "custom" behandeln
return "custom";
}
