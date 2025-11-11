#include "BehaviorManager.h"
#include "LayoutManager.h"
#include "LayoutBackend.h"

#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// ----------------------------------------------------------
// Konstruktor
// ----------------------------------------------------------
BehaviorManager::BehaviorManager(FlagManager* flagMgr,
                                 TextManager* textMgr,
                                 DefineManager* defineMgr,
                                 LayoutManager* layoutMgr)
    : m_flagMgr(flagMgr)
    , m_textMgr(textMgr)
    , m_defineMgr(defineMgr)
    , m_layoutMgr(layoutMgr)
{
}

// ----------------------------------------------------------
// High-Level Behavior API – vorerst NO-OP
// ----------------------------------------------------------
BehaviorInfo BehaviorManager::resolveBehavior(const ControlData& ctrl) const
{
    Q_UNUSED(ctrl);
    BehaviorInfo info;
    return info;
}

void BehaviorManager::applyBehavior(ControlData& ctrl) const
{
    Q_UNUSED(ctrl);
}

// ----------------------------------------------------------
// Flags & Types laden (ehemals LayoutManager::refreshFromFiles)
// ----------------------------------------------------------
void BehaviorManager::refreshFlagsFromFiles()
{
    if (!m_layoutMgr) {
        qWarning() << "[BehaviorManager] Kein LayoutManager gesetzt – "
                      "kann Flags nicht laden.";
        return;
    }

    LayoutBackend& backend = m_layoutMgr->backend();

    const QJsonObject winObj  = backend.loadWindowFlags();
    const QJsonObject ctrlObj = backend.loadControlFlags();
    const QJsonObject typeObj = backend.loadWindowTypes();

    m_windowFlags.clear();
    m_controlFlags.clear();
    m_windowTypes.clear();
    m_windowRules = QJsonObject{};
    m_controlRules = QJsonObject{};
    m_windowRulesLoaded = false;
    m_controlRulesLoaded = false;

    // -------------------------
    // Window Flags (links-shifted)
    // -------------------------
    for (auto it = winObj.constBegin(); it != winObj.constEnd(); ++it)
    {
        QString val = it.value().toString().trimmed().toUpper();
        if (val.startsWith("0X")) val.remove(0, 2);
        if (val.endsWith("L")) val.chop(1);

        bool ok = false;
        quint32 raw = val.toUInt(&ok, 16);
        if (ok)
            m_windowFlags[it.key()] = (raw << 16);
        else
            qWarning().noquote()
                << "[BehaviorManager] Ungültiger Window-Flag-Wert:"
                << it.key() << "=" << it.value().toVariant().toString();
    }

    // -------------------------
    // Control Flags
    // -------------------------
    for (auto it = ctrlObj.constBegin(); it != ctrlObj.constEnd(); ++it)
    {
        QString val = it.value().toString().trimmed().toUpper();
        if (val.startsWith("0X")) val.remove(0, 2);
        if (val.endsWith("L")) val.chop(1);

        bool ok = false;
        quint32 raw = val.toUInt(&ok, 16);
        if (ok)
            m_controlFlags[it.key()] = raw;
        else
            qWarning().noquote()
                << "[BehaviorManager] Ungültiger Control-Flag-Wert:"
                << it.key() << "=" << it.value().toString();
    }

    // -------------------------
    // Window Types
    // -------------------------
    for (auto it = typeObj.constBegin(); it != typeObj.constEnd(); ++it)
    {
        QString val = it.value().toString().trimmed().toUpper();
        if (val.startsWith("0X")) val.remove(0, 2);
        if (val.endsWith("L")) val.chop(1);

        bool ok = false;
        quint32 raw = val.toUInt(&ok, 16);
        if (ok)
            m_windowTypes[it.key()] = raw;
        else
            qWarning().noquote()
                << "[BehaviorManager] Ungültiger WindowType-Wert:"
                << it.key() << "=" << it.value().toString();
    }

    qInfo().noquote()
        << QString("[BehaviorManager] Flags aktualisiert: Window=%1  Control=%2  Types=%3")
               .arg(m_windowFlags.size())
               .arg(m_controlFlags.size())
               .arg(m_windowTypes.size());
}

// ----------------------------------------------------------
// Window-Flags validieren (ehemals LayoutManager::validateWindowFlags)
// ----------------------------------------------------------
void BehaviorManager::validateWindowFlags(WindowData* win)
{
    if (!win) return;

    quint32 parsedWin = win->flagsMask;

    if (m_windowFlags.isEmpty()) {
        qWarning() << "[BehaviorManager] Keine Window-Flags geladen!";
        return;
    }

    const quint32 lowWord  = parsedWin & 0x0000FFFFu;
    const quint32 highWord = parsedWin & 0xFFFF0000u;

    if ((lowWord != 0u && highWord == 0u) ||
        (lowWord != 0u && highWord != 0u))
    {
        qInfo().noquote()
        << "[BehaviorManager] Shift korrigiert Fenster"
        << win->name
        << "Maske alt=0x" << QString::number(parsedWin, 16).toUpper();

        const quint32 shiftedLow = (lowWord << 16);
        parsedWin = highWord | shiftedLow;

        win->flagsMask = parsedWin;

        qInfo().noquote()
            << "[BehaviorManager] → Maske neu=0x"
            << QString::number(parsedWin, 16).toUpper();
    }

    QStringList matchedWindowFlags;
    quint32 knownWindowBits = 0;

    for (auto it = m_windowFlags.constBegin(); it != m_windowFlags.constEnd(); ++it)
    {
        const quint32 flagMask = it.value();
        knownWindowBits |= flagMask;

        if ((parsedWin & flagMask) == flagMask)
            matchedWindowFlags << it.key();
    }

    const quint32 unknownBits = parsedWin & ~knownWindowBits;
    win->valid        = (unknownBits == 0);
    win->resolvedMask = matchedWindowFlags;
    win->flagsMask    = parsedWin;

    if (!win->valid)
    {
        qWarning().noquote()
        << "[BehaviorManager] Ungültige Fensterbits:"
        << QString("0x%1 → unbekannt: 0x%2")
                .arg(QString::number(parsedWin, 16).toUpper())
                .arg(QString::number(unknownBits, 16).toUpper());
    }
}

// ----------------------------------------------------------
// Control-Flags validieren (ehemals LayoutManager::validateControlFlags)
// ----------------------------------------------------------
void BehaviorManager::validateControlFlags(ControlData* ctrl)
{
    if (!ctrl) return;

    const quint32 parsedCtrlMask = ctrl->flagsMask;
    const quint32 ctrlLow = parsedCtrlMask & 0x0000FFFF;

    if (m_controlFlags.isEmpty()) {
        qWarning() << "[BehaviorManager] Keine Control-Flags geladen!";
        return;
    }

    QStringList matchedControlFlags;
    quint32 knownBits = 0;

    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
    {
        const quint32 bit = it.value();
        knownBits |= bit;
        if ((ctrlLow & bit) == bit)
            matchedControlFlags << it.key();
    }

    const quint32 unknownBits = ctrlLow & ~knownBits;

    if (ctrlLow == 0)
        ctrl->valid = true;
    else
        ctrl->valid = (unknownBits == 0);

    ctrl->resolvedMask = matchedControlFlags;
    ctrl->flagsMask    = parsedCtrlMask;

    if (!ctrl->valid)
    {
        qWarning().noquote() << "[BehaviorManager] Unbekannte Control-Bits:"
                             << QString("0x%1 (Low-Word: 0x%2)")
                                    .arg(QString::number(parsedCtrlMask, 16).toUpper())
                                    .arg(QString::number(ctrlLow, 16).toUpper());
    }
}

// ----------------------------------------------------------
// Update-Wrapper (ehemals LayoutManager::update*)
// ----------------------------------------------------------
void BehaviorManager::updateWindowFlags(const std::shared_ptr<WindowData>& wnd)
{
    if (!wnd)
        return;

    validateWindowFlags(wnd.get());
}

void BehaviorManager::updateControlFlags(const std::shared_ptr<ControlData>& ctrl)
{
    if (!ctrl)
        return;

    validateControlFlags(ctrl.get());
    // generateUnknownControls wird später auf allen Controls ausgeführt,
    // hier NICHT einzeln, um Spam zu vermeiden.
}

// ----------------------------------------------------------
// ControlType-Analyse (ehemals LayoutManager::analyzeControlTypes)
// ----------------------------------------------------------
void BehaviorManager::analyzeControlTypes(const std::vector<std::shared_ptr<WindowData>>& windows)
{
    m_controlTypeProperties.clear();

    for (const auto& wnd : windows)
    {
        if (!wnd) continue;

        for (const auto& ctrl : wnd->controls)
        {
            if (!ctrl) continue;
            auto& props = m_controlTypeProperties[ctrl->type];

            if (!ctrl->id.isEmpty()) props.insert("ID");
            if (!ctrl->texture.isEmpty()) props.insert("Texture");
            props.insert("X1"); props.insert("Y1"); props.insert("X2"); props.insert("Y2");
            if (!ctrl->flagsHex.isEmpty()) props.insert("FlagsHex");
            if (ctrl->flagsMask != 0u) props.insert("FlagsMask");
            if (ctrl->color.isValid()) props.insert("Color");
        }
    }

    qInfo().noquote()
        << QString("[BehaviorManager] ControlType-Analyse abgeschlossen: %1 Typen erkannt.")
               .arg(m_controlTypeProperties.size());
}

bool BehaviorManager::allowsPropertyForType(const QString& type, const QString& property) const
{
    if (type.isEmpty() || property.isEmpty())
        return false;

    if (m_controlTypeProperties.isEmpty()) {
        // const_cast ist hässlich, aber pragmatisch wie im alten Code
        const_cast<BehaviorManager*>(this)->analyzeControlTypes(std::vector<std::shared_ptr<WindowData>>{});
        // Hinweis: in der Praxis solltest du analyzeControlTypes mit echten Windows
        // aus LayoutManager aufrufen, z.B. in processLayout.
    }

    const auto it = m_controlTypeProperties.constFind(type);
    if (it == m_controlTypeProperties.constEnd())
        return false;

    if (it.value().contains(property))
        return true;

    for (const QString& prop : it.value())
    {
        if (prop.compare(property, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

// ----------------------------------------------------------
// Default Window Flag Rules (ehemals generateDefaultWindowFlagRules)
// ----------------------------------------------------------
void BehaviorManager::generateDefaultWindowFlagRules()
{
    if (!m_layoutMgr) return;
    LayoutBackend& backend = m_layoutMgr->backend();

    QJsonObject root;
    QJsonObject defaults;
    QJsonArray valid;

    for (auto it = m_windowFlags.constBegin(); it != m_windowFlags.constEnd(); ++it)
    {
        const QString& flagName = it.key();
        if (flagName.startsWith("WBS_"))
            valid.append(flagName);
    }

    defaults["valid"] = valid;

    QJsonObject exclusive;

    auto addExclusive = [&exclusive](const QString &a, const QStringList &others) {
        QJsonArray arr;
        for (const QString &o : others)
            arr.append(o);
        exclusive.insert(a, arr);
    };

    if (m_windowFlags.contains("WBS_NOFRAME") && m_windowFlags.contains("WBS_THICKFRAME"))
    {
        addExclusive("WBS_NOFRAME",   {"WBS_THICKFRAME"});
        addExclusive("WBS_THICKFRAME",{"WBS_NOFRAME"});
    }

    if (m_windowFlags.contains("WBS_HORI") && m_windowFlags.contains("WBS_VERT"))
    {
        addExclusive("WBS_HORI", {"WBS_VERT"});
        addExclusive("WBS_VERT", {"WBS_HORI"});
    }

    if (!exclusive.isEmpty())
        defaults["exclusive"] = exclusive;

    root["Default"] = defaults;

    backend.saveWindowFlagRules(root);

    qInfo() << "[BehaviorManager] Default window_flag_rules.json generiert ("
            << valid.size() << " Flags eingetragen ).";
}

// ----------------------------------------------------------
// Default Control Flag Rules (ehemals generateDefaultControlFlagRules)
// ----------------------------------------------------------
void BehaviorManager::generateDefaultControlFlagRules()
{
    if (!m_layoutMgr) return;
    LayoutBackend& backend = m_layoutMgr->backend();

    QJsonObject root;
    QJsonObject defaults;
    QJsonArray valid;

    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
    {
        const QString& flagName = it.key();
        valid.append(flagName);
    }

    defaults["valid"] = valid;

    QJsonObject exclusive;

    auto addExclusive = [&exclusive](const QString& a, const QStringList& others)
    {
        QJsonArray arr;
        for (const QString& o : others)
            arr.append(o);
        exclusive.insert(a, arr);
    };

    if (m_controlFlags.contains("EBS_LEFT") &&
        m_controlFlags.contains("EBS_CENTER") &&
        m_controlFlags.contains("EBS_RIGHT"))
    {
        addExclusive("EBS_LEFT",   {"EBS_CENTER", "EBS_RIGHT"});
        addExclusive("EBS_CENTER", {"EBS_LEFT",   "EBS_RIGHT"});
        addExclusive("EBS_RIGHT",  {"EBS_LEFT",   "EBS_CENTER"});
    }

    if (m_controlFlags.contains("ES_LEFT") &&
        m_controlFlags.contains("ES_CENTER") &&
        m_controlFlags.contains("ES_RIGHT"))
    {
        addExclusive("ES_LEFT",   {"ES_CENTER", "ES_RIGHT"});
        addExclusive("ES_CENTER", {"ES_LEFT",   "ES_RIGHT"});
        addExclusive("ES_RIGHT",  {"ES_LEFT",   "ES_CENTER"});
    }

    if (m_controlFlags.contains("SS_LEFT") &&
        m_controlFlags.contains("SS_CENTER") &&
        m_controlFlags.contains("SS_RIGHT"))
    {
        addExclusive("SS_LEFT",   {"SS_CENTER", "SS_RIGHT"});
        addExclusive("SS_CENTER", {"SS_LEFT",   "SS_RIGHT"});
        addExclusive("SS_RIGHT",  {"SS_LEFT",   "SS_CENTER"});
    }

    if (!exclusive.isEmpty())
        defaults["exclusive"] = exclusive;

    root["Default"] = defaults;

    backend.saveControlFlagRules(root);

    qInfo().noquote()
        << "[BehaviorManager] Default control_flag_rules.json generiert ("
        << valid.size() << " Flags eingetragen ).";
}

// ----------------------------------------------------------
// Rules laden + Cache bauen (ehemals reload*/rebuildValidFlagCache)
// ----------------------------------------------------------
void BehaviorManager::reloadWindowFlagRules()
{
    if (!m_layoutMgr) return;
    LayoutBackend& backend = m_layoutMgr->backend();

    m_windowRules = backend.loadWindowFlagRules();
    m_windowRulesLoaded = true;

    if (m_windowRules.isEmpty())
    {
        qInfo() << "[BehaviorManager] Keine window_flag_rules gefunden – erstelle Defaults.";
        generateDefaultWindowFlagRules();
        m_windowRules = backend.loadWindowFlagRules();
    }
}

void BehaviorManager::reloadControlFlagRules()
{
    if (!m_layoutMgr) return;
    LayoutBackend& backend = m_layoutMgr->backend();

    m_controlRules = backend.loadControlFlagRules();
    m_controlRulesLoaded = true;

    if (m_controlRules.isEmpty())
    {
        qInfo() << "[BehaviorManager] Keine control_flag_rules gefunden – erstelle Defaults.";
        generateDefaultControlFlagRules();
        m_controlRules = backend.loadControlFlagRules();
    }
}

QJsonObject BehaviorManager::getWindowFlagRules() const
{
    if (!m_windowRulesLoaded)
        const_cast<BehaviorManager*>(this)->reloadWindowFlagRules();
    return m_windowRules;
}

QJsonObject BehaviorManager::getControlFlagRules() const
{
    if (!m_controlRulesLoaded)
        const_cast<BehaviorManager*>(this)->reloadControlFlagRules();
    return m_controlRules;
}

void BehaviorManager::rebuildValidFlagCache()
{
    m_validFlagsByType.clear();

    const auto addRules = [this](const QJsonObject& rules, const QString& prefix) {
        for (auto it = rules.constBegin(); it != rules.constEnd(); ++it)
        {
            const QJsonObject obj = it.value().toObject();
            const QJsonArray valid = obj.value("valid").toArray();
            if (valid.isEmpty())
                continue;

            QSet<QString> entries;
            for (const auto& v : valid)
                entries.insert(v.toString());

            if (!entries.isEmpty())
                m_validFlagsByType.insert(prefix + it.key(), entries);
        }
    };

    addRules(getWindowFlagRules(), QStringLiteral("window:"));
    addRules(getControlFlagRules(), QStringLiteral("control:"));
}

// ----------------------------------------------------------
// Allowed-Flags für Window/Control-Typen
// ----------------------------------------------------------
QSet<QString> BehaviorManager::allowedWindowFlags(const QString& typeName) const
{
    if (m_validFlagsByType.isEmpty())
        const_cast<BehaviorManager*>(this)->rebuildValidFlagCache();

    if (m_validFlagsByType.isEmpty())
        return {};

    const QString directKey  = QStringLiteral("window:%1").arg(typeName);
    const QString defaultKey = QStringLiteral("window:Default");

    if (m_validFlagsByType.contains(directKey))
        return m_validFlagsByType.value(directKey);

    return m_validFlagsByType.value(defaultKey);
}

QSet<QString> BehaviorManager::allowedControlFlags(const QString& typeName) const
{
    if (m_validFlagsByType.isEmpty())
        const_cast<BehaviorManager*>(this)->rebuildValidFlagCache();

    if (m_validFlagsByType.isEmpty())
        return {};

    const QString directKey  = QStringLiteral("control:%1").arg(typeName);
    const QString defaultKey = QStringLiteral("control:Default");

    if (m_validFlagsByType.contains(directKey))
        return m_validFlagsByType.value(directKey);

    return m_validFlagsByType.value(defaultKey);
}

// ----------------------------------------------------------
// Unknown Control Bits (ehemals generateUnknownControls)
// ----------------------------------------------------------
void BehaviorManager::generateUnknownControls(const std::vector<std::shared_ptr<WindowData>>& windows)
{
    if (!m_layoutMgr) return;
    LayoutBackend& backend = m_layoutMgr->backend();

    m_unknownControlBits.clear();

    if (windows.empty() || m_controlFlags.isEmpty()) {
        backend.saveUndefinedControlFlags(QJsonObject{});
        return;
    }

    quint32 knownBits = 0;
    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
        knownBits |= it.value();

    for (const auto& wnd : windows)
    {
        if (!wnd)
            continue;

        for (const auto& ctrl : wnd->controls)
        {
            if (!ctrl)
                continue;

            const quint32 ctrlLow = ctrl->flagsMask & 0x0000FFFF;

            if (ctrlLow == 0)
                continue;

            const quint32 unknownMask = ctrlLow & ~knownBits;
            if (unknownMask == 0u)
                continue;

            const QString maskHex = QStringLiteral("0x%1").arg(unknownMask, 0, 16).toUpper();
            QString identifier = ctrl->id;
            if (identifier.isEmpty())
                identifier = ctrl->rawHeader.trimmed();
            if (identifier.isEmpty())
                identifier = QStringLiteral("[%1/%2]").arg(wnd->name).arg(ctrl->type);

            m_unknownControlBits[ctrl->type][maskHex].insert(identifier);
        }
    }

    if (m_unknownControlBits.isEmpty()) {
        backend.saveUndefinedControlFlags(QJsonObject{});
        return;
    }

    QJsonObject root;
    for (auto typeIt = m_unknownControlBits.cbegin(); typeIt != m_unknownControlBits.cend(); ++typeIt)
    {
        QJsonArray entries;
        for (auto maskIt = typeIt.value().cbegin(); maskIt != typeIt.value().cend(); ++maskIt)
        {
            QJsonObject entry;
            entry.insert("mask", maskIt.key());
            QJsonArray controls;
            for (const auto& id : maskIt.value())
                controls.append(id);
            entry.insert("controls", controls);
            entries.append(entry);
        }
        root.insert(typeIt.key(), entries);
    }

    if (!backend.saveUndefinedControlFlags(root))
        qWarning().noquote() << "[BehaviorManager] Konnte undefined_control_flags.json nicht aktualisieren.";
}
