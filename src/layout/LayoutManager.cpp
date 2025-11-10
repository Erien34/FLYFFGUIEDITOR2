#include "LayoutManager.h"
#include "LayoutBackend.h"
#include "model/WindowData.h"
#include "model/ControlData.h"
#include <QDebug>
#include <QFileInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <vector>

// -------------------------------------------------------------
// Hilfsfunktion
// -------------------------------------------------------------
QString LayoutManager::unquote(const QString& s) const
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.mid(1, s.size() - 2);
    return s;
}

// -------------------------------------------------------------
// Konstruktoren
// -------------------------------------------------------------
LayoutManager::LayoutManager(LayoutParser& parser, LayoutBackend& backend)
    : m_parser(parser), m_backend(backend)
{
    connect(&m_parser, &LayoutParser::tokensReady, this, &LayoutManager::tokensReady);
}

// -------------------------------------------------------------
// Flags aus JSON laden (Fenster / Controls / Types)
// -------------------------------------------------------------
void LayoutManager::refreshFromFiles(const QString& wndPath, const QString& ctrlPath)
{
    Q_UNUSED(wndPath)
    Q_UNUSED(ctrlPath)

    // ----------------------------------------------------
    // 🔹 Daten aus dem Backend laden
    // ----------------------------------------------------
    const QJsonObject winObj  = m_backend.loadWindowFlags();
    const QJsonObject ctrlObj = m_backend.loadControlFlags();
    const QJsonObject typeObj = m_backend.loadWindowTypes();

    m_windowFlags.clear();
    m_controlFlags.clear();
    m_windowTypes.clear();
    m_windowRules = QJsonObject{};
    m_controlRules = QJsonObject{};
    m_windowRulesLoaded = false;
    m_controlRulesLoaded = false;

    // ----------------------------------------------------
    // 🪟 Window Flags
    // ----------------------------------------------------
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
                << "[LayoutManager] Ungültiger Window-Flag-Wert:"
                << it.key() << "=" << it.value().toVariant().toString();
    }

    // ----------------------------------------------------
    // Control Flags (ebenfalls links-shifted)
    // ----------------------------------------------------
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
                << "[LayoutManager] Ungültiger Control-Flag-Wert:"
                << it.key() << "=" << it.value().toString();
    }

    // ----------------------------------------------------
    // 🧩 Window Types
    // ----------------------------------------------------
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
                << "[LayoutManager] Ungültiger WindowType-Wert:"
                << it.key() << "=" << it.value().toString();
    }

    qInfo().noquote()
        << QString("[LayoutManager] Flags aktualisiert: Window=%1  Control=%2  Types=%3")
               .arg(m_windowFlags.size())
               .arg(m_controlFlags.size())
               .arg(m_windowTypes.size());
}


// -------------------------------------------------------------
// Parserdaten übernehmen
// -------------------------------------------------------------
void LayoutManager::refreshFromParser()
{
    m_windows.clear();
    const auto tokenMap = TokenData::instance().all();

    for (auto it = tokenMap.cbegin(); it != tokenMap.cend(); ++it)
    {
        const QList<Token>& tokens = it.value();
        if (tokens.isEmpty())
            continue;

        if (it.key().trimmed().isEmpty())
            continue;

        auto win = std::make_shared<WindowData>();
        win->name = it.key();


        int i = 0;

        if (win->flagsMask == 0x164)
        {
            qInfo().noquote()
            << "[Debug] Fenster mit 0x164 gefunden:"
            << win->name
            << "Zeile:" << tokens[i-1].orderIndex
            << "Header:" << tokens[i-1].value;
        }
        // =========================================================
        // Window-Header
        // =========================================================
        if (i < tokens.size() && tokens[i].type == "WindowHeader")
        {
            const QStringList p = tokens[i++].value.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (p.size() >= 8)
            {
                win->texture   = p[1];
                win->titletext = p[2];
                win->modus     = p[3].toInt();
                win->width     = p[4].toInt();
                win->height    = p[5].toInt();
                win->flagsHex  = p[6];
                win->mod       = p[7].toInt();

                bool ok = false;
                QString clean = win->flagsHex.trimmed().toUpper();
                if (clean.startsWith("0X")) clean.remove(0, 2);
                if (clean.endsWith("L")) clean.chop(1);
                win->flagsMask = clean.toUInt(&ok, 16);
                if (!ok) {
                    win->flagsMask = 0;
                    qWarning().noquote() << "[LayoutManager] Ungültiger Window-Flagwert:"
                                         << win->flagsHex << "bei" << win->name;
                }
            }
        }

        // =========================================================
        // Controls einlesen
        // =========================================================
        while (i < tokens.size())
        {
            if (tokens[i].type != "ControlHeader") { ++i; continue; }

            auto ctrl = std::make_shared<ControlData>();
            const QStringList p = tokens[i].value.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            ctrl->rawHeader = tokens[i].value;

            if (p.size() >= 9)
            {
                ctrl->type     = p[0];
                ctrl->id       = p[1];
                ctrl->texture  = unquote(p[2]);
                ctrl->x1       = p[4].toInt();
                ctrl->y1       = p[5].toInt();
                ctrl->x2       = p[6].toInt();
                ctrl->y2       = p[7].toInt();
                ctrl->flagsHex = p[8];

                bool ok = false;
                QString clean = ctrl->flagsHex.trimmed().toUpper();
                if (clean.startsWith("0X")) clean.remove(0, 2);
                if (clean.endsWith("L")) clean.chop(1);
                ctrl->flagsMask = clean.toUInt(&ok, 16);
                if (!ok) ctrl->flagsMask = 0;
            }

            win->controls.push_back(ctrl);
            ++i;
        }

        m_windows.push_back(win);
    }

    qInfo().noquote() << QString("[LayoutManager] Parserdaten übernommen → %1 Fenster.")
                             .arg(m_windows.size());
}

// -------------------------------------------------------------
// Layout validieren
// -------------------------------------------------------------
void LayoutManager::processLayout()
{
    qInfo() << "[LayoutManager] Verarbeite Layouts...";

    for (auto& wndPtr : m_windows)
    {
        if (!wndPtr) continue;
         validateWindowFlags(wndPtr.get());

        for (auto& ctrlPtr : wndPtr->controls)
        {
            if (ctrlPtr) validateControlFlags(ctrlPtr.get());
        }
    }

    analyzeControlTypes();
    generateUnknownControls();
    rebuildValidFlagCache();

    qInfo() << "[LayoutManager] Validierung abgeschlossen.";
}

// -------------------------------------------------------------
// Validate Window Flags
// -------------------------------------------------------------
void LayoutManager::validateWindowFlags(WindowData* win)
{
    if (!win) return;

    quint32 parsedWin = win->flagsMask;

    if (m_windowFlags.isEmpty()) {
        qWarning() << "[LayoutManager] Keine Window-Flags geladen!";
        return;
    }

    // 🔹 Zerlegen in High- und Low-Word
    const quint32 lowWord  = parsedWin & 0x0000FFFFu;
    const quint32 highWord = parsedWin & 0xFFFF0000u;

    // 🔹 Prüfen, ob verrutschte Bits vorhanden sind:
    // Fall 1 → Nur Low-Word-Bits (kein High-Word gesetzt)
    // Fall 2 → Mischung aus High- und Low-Word (Low-Word-Bits zusätzlich aktiv)
    if ((lowWord != 0u && highWord == 0u) ||
        (lowWord != 0u && highWord != 0u))
    {
        qInfo().noquote()
        << "[LayoutManager] Shift korrigiert Fenster"
        << win->name
        << "Maske alt=0x" << QString::number(parsedWin, 16).toUpper();

        // 🔸 Low-Word-Bits um 16 nach oben verschieben
        const quint32 shiftedLow = (lowWord << 16);
        parsedWin = highWord | shiftedLow;

        win->flagsMask = parsedWin;

        qInfo().noquote()
            << "[LayoutManager] → Maske neu=0x"
            << QString::number(parsedWin, 16).toUpper();
    }

    // 🔹 Jetzt normale Validierung gegen bekannte Window-Flags
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
        << "[LayoutManager] Ungültige Fensterbits:"
        << QString("0x%1 → unbekannt: 0x%2")
                .arg(QString::number(parsedWin, 16).toUpper())
                .arg(QString::number(unknownBits, 16).toUpper());
    }
}
// -------------------------------------------------------------
// Validate Control Flags
// -------------------------------------------------------------
void LayoutManager::validateControlFlags(ControlData* ctrl)
{
    if (!ctrl) return;

    const quint32 parsedCtrlMask = ctrl->flagsMask;

    // 🔹 Nur Low-Word prüfen (untere 16 Bits)
    const quint32 ctrlLow = parsedCtrlMask & 0x0000FFFF;

    if (m_controlFlags.isEmpty()) {
        qWarning() << "[LayoutManager] Keine Control-Flags geladen!";
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

    // 🔹 Bits, die NICHT in control_flags.json vorkommen
    const quint32 unknownBits = ctrlLow & ~knownBits;

    // 🔹 Falls Low-Word leer ist, gilt das Control als „ohne Styles → gültig“
    if (ctrlLow == 0)
        ctrl->valid = true;
    else
        ctrl->valid = (unknownBits == 0);

    ctrl->resolvedMask = matchedControlFlags;
    ctrl->flagsMask = parsedCtrlMask;

    if (!ctrl->valid)
    {
        qWarning().noquote() << "[LayoutManager] Unbekannte Control-Bits:"
                             << QString("0x%1 (Low-Word: 0x%2)")
                                    .arg(QString::number(parsedCtrlMask, 16).toUpper())
                                    .arg(QString::number(ctrlLow, 16).toUpper());
    }
}

// -------------------------------------------------------------
// Analyze Control Types
// -------------------------------------------------------------
void LayoutManager::analyzeControlTypes()
{
    m_controlTypeProperties.clear();

    for (const auto& wnd : m_windows)
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
        << QString("[LayoutManager] ControlType-Analyse abgeschlossen: %1 Typen erkannt.")
               .arg(m_controlTypeProperties.size());
}

bool LayoutManager::allowsPropertyForType(const QString& type, const QString& property) const
{
    if (type.isEmpty() || property.isEmpty())
        return false;

    if (m_controlTypeProperties.isEmpty())
        const_cast<LayoutManager*>(this)->analyzeControlTypes();

    const auto it = m_controlTypeProperties.constFind(type);
    if (it == m_controlTypeProperties.constEnd())
        return false;

    if (it.value().contains(property))
        return true;

    // Fallback auf Groß-/Kleinschreibung ignorierend
    for (const QString& prop : it.value())
    {
        if (prop.compare(property, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}
// -------------------------------------------------------------
// Generate Default Window Flag Rules
// -------------------------------------------------------------
void LayoutManager::generateDefaultWindowFlagRules()
{
    QJsonObject root;
    QJsonObject defaults;
    QJsonArray valid;

    // 🔹 Alle bekannten Window-Flags automatisch aus m_windowFlags übernehmen
    for (auto it = m_windowFlags.constBegin(); it != m_windowFlags.constEnd(); ++it)
    {
        const QString& flagName = it.key();
        if (flagName.startsWith("WBS_")) // nur echte Window-Flags
            valid.append(flagName);
    }

    defaults["valid"] = valid;

    // 🔹 Optional: sinnvolle Exklusivregeln (z. B. Rahmen oder Orientierung)
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

    // 🔹 Root-Objekt zusammenbauen
    root["Default"] = defaults;

    // 🔹 Speichern über Backend
    m_backend.saveWindowFlagRules(root);

    qInfo() << "[LayoutManager] Default window_flag_rules.json generiert ("
            << valid.size() << " Flags eingetragen ).";
}
// -------------------------------------------------------------
// Generate Default Control Flag Rules
// -------------------------------------------------------------
void LayoutManager::generateDefaultControlFlagRules()
{
    QJsonObject root;
    QJsonObject defaults;
    QJsonArray valid;

    // 🔹 Alle bekannten Control-Flags aus m_controlFlags übernehmen
    //    (egal welches Prefix – was hier drin ist, gilt als grundsätzlich erlaubt)
    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
    {
        const QString& flagName = it.key();
        valid.append(flagName);
    }

    defaults["valid"] = valid;

    // 🔹 Optionale Exklusivgruppen (z. B. Textausrichtung)
    QJsonObject exclusive;

    auto addExclusive = [&exclusive](const QString& a, const QStringList& others)
    {
        QJsonArray arr;
        for (const QString& o : others)
            arr.append(o);
        exclusive.insert(a, arr);
    };

    // EBS_* (FlyFF-Edit-Styles)
    if (m_controlFlags.contains("EBS_LEFT") &&
        m_controlFlags.contains("EBS_CENTER") &&
        m_controlFlags.contains("EBS_RIGHT"))
    {
        addExclusive("EBS_LEFT",   {"EBS_CENTER", "EBS_RIGHT"});
        addExclusive("EBS_CENTER", {"EBS_LEFT",   "EBS_RIGHT"});
        addExclusive("EBS_RIGHT",  {"EBS_LEFT",   "EBS_CENTER"});
    }

    // Optional: Win32-Edit-Styles ES_LEFT/CENTER/RIGHT
    if (m_controlFlags.contains("ES_LEFT") &&
        m_controlFlags.contains("ES_CENTER") &&
        m_controlFlags.contains("ES_RIGHT"))
    {
        addExclusive("ES_LEFT",   {"ES_CENTER", "ES_RIGHT"});
        addExclusive("ES_CENTER", {"ES_LEFT",   "ES_RIGHT"});
        addExclusive("ES_RIGHT",  {"ES_LEFT",   "ES_CENTER"});
    }

    // Optional: Win32-Static-Styles SS_LEFT/CENTER/RIGHT
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

    // 🔹 Root-Objekt zusammensetzen
    root["Default"] = defaults;

    // 🔹 Speichern über Backend
    m_backend.saveControlFlagRules(root);

    qInfo().noquote()
        << "[LayoutManager] Default control_flag_rules.json generiert ("
        << valid.size() << " Flags eingetragen ).";
}
// -------------------------------------------------------------
// Serialize Layout (unverändert, nicht kürzen!)
// -------------------------------------------------------------
QString LayoutManager::serializeLayout() const
{
    QString out;
    out.reserve(131072);
    const auto& allWindows = TokenData::instance().all();

    for (auto it = allWindows.cbegin(); it != allWindows.cend(); ++it)
    {
        const QList<Token>& tokens = it.value();
        if (tokens.isEmpty())
            continue;

        int i = 0;
        while (i < tokens.size() && tokens[i].type != "WindowHeader") ++i;
        if (i >= tokens.size()) continue;

        const QString header = tokens[i++].value.trimmed();
        out += header + "\r\n";

        QString title, help;
        int textCount = 0;
        while (i < tokens.size() && tokens[i].type != "ControlHeader")
        {
            if (tokens[i].type == "Text")
            {
                if (textCount == 0) title = tokens[i].value.trimmed();
                else if (textCount == 1) help = tokens[i].value.trimmed();
                ++textCount;
            }
            ++i;
        }

        out += "{\r\n    // Title String\r\n";
        if (!title.isEmpty()) out += "    " + title + "\r\n";
        out += "}\r\n";

        out += "{\r\n    // Help Key\r\n";
        if (!help.isEmpty()) out += "    " + help + "\r\n";
        out += "}\r\n";

        out += "{\r\n";
        while (i < tokens.size())
        {
            if (tokens[i].type != "ControlHeader") { ++i; continue; }

            const QString ctrlHeader = tokens[i++].value.trimmed();
            out += "    " + ctrlHeader + "\r\n";

            QString ctrlTitle, ctrlTooltip;
            int tcount = 0;
            while (i < tokens.size() && tokens[i].type != "ControlHeader")
            {
                if (tokens[i].type == "Text")
                {
                    if (tcount == 0) ctrlTitle = tokens[i].value.trimmed();
                    else if (tcount == 1) ctrlTooltip = tokens[i].value.trimmed();
                    ++tcount;
                }
                ++i;
            }

            out += "    {\r\n        // Title String\r\n";
            if (!ctrlTitle.isEmpty()) out += "        " + ctrlTitle + "\r\n";
            out += "    }\r\n";

            out += "    {\r\n        // ToolTip\r\n";
            if (!ctrlTooltip.isEmpty()) out += "        " + ctrlTooltip + "\r\n";
            out += "    }\r\n";
        }
        out += "}\r\n\r\n";
    }
    return out;
}


QJsonObject LayoutManager::getWindowFlagRules() const
{
    if (!m_windowRulesLoaded)
        const_cast<LayoutManager*>(this)->reloadWindowFlagRules();
    return m_windowRules;
}

QJsonObject LayoutManager::getControlFlagRules() const
{
    if (!m_controlRulesLoaded)
        const_cast<LayoutManager*>(this)->reloadControlFlagRules();
    return m_controlRules;
}

QSet<QString> LayoutManager::allowedWindowFlags(const QString& typeName) const
{
    if (m_validFlagsByType.isEmpty())
        const_cast<LayoutManager*>(this)->rebuildValidFlagCache();

    if (m_validFlagsByType.isEmpty())
        return {};

    const QString directKey = QStringLiteral("window:%1").arg(typeName);
    const QString defaultKey = QStringLiteral("window:Default");

    if (m_validFlagsByType.contains(directKey))
        return m_validFlagsByType.value(directKey);

    return m_validFlagsByType.value(defaultKey);
}

QSet<QString> LayoutManager::allowedControlFlags(const QString& typeName) const
{
    if (m_validFlagsByType.isEmpty())
        const_cast<LayoutManager*>(this)->rebuildValidFlagCache();

    if (m_validFlagsByType.isEmpty())
        return {};

    const QString directKey = QStringLiteral("control:%1").arg(typeName);
    const QString defaultKey = QStringLiteral("control:Default");

    if (m_validFlagsByType.contains(directKey))
        return m_validFlagsByType.value(directKey);

    return m_validFlagsByType.value(defaultKey);
}

void LayoutManager::updateWindowFlags(const std::shared_ptr<WindowData>& wnd)
{
    if (!wnd)
        return;

    validateWindowFlags(wnd.get());
}

void LayoutManager::updateControlFlags(const std::shared_ptr<ControlData>& ctrl)
{
    if (!ctrl)
        return;

    validateControlFlags(ctrl.get());
    generateUnknownControls();
}

std::shared_ptr<WindowData> LayoutManager::findWindow(const QString& name) const
{
    for (const auto& wnd : m_windows)
    {
        if (wnd && wnd->name.compare(name, Qt::CaseInsensitive) == 0)
            return wnd;
    }
    return nullptr;
}

void LayoutManager::generateUnknownControls()
{
    m_unknownControlBits.clear();

    if (m_windows.empty() || m_controlFlags.isEmpty()) {
        m_backend.saveUndefinedControlFlags(QJsonObject{});
        return;
    }

    // 🔹 Alle bekannten Bits (nur Low-Word)
    quint32 knownBits = 0;
    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
        knownBits |= it.value();

    for (const auto& wnd : m_windows)
    {
        if (!wnd)
            continue;

        for (const auto& ctrl : wnd->controls)
        {
            if (!ctrl)
                continue;

            const quint32 ctrlLow = ctrl->flagsMask & 0x0000FFFF;

            // 🔹 Wenn keine Styles im Low-Word → gültig, überspringen
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

    // 🔹 Export
    if (m_unknownControlBits.isEmpty()) {
        m_backend.saveUndefinedControlFlags(QJsonObject{});
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

    if (!m_backend.saveUndefinedControlFlags(root))
        qWarning().noquote() << "[LayoutManager] Konnte undefined_control_flags.json nicht aktualisieren.";
}

void LayoutManager::reloadWindowFlagRules()
{
    m_windowRules = m_backend.loadWindowFlagRules();
    m_windowRulesLoaded = true;

    if (m_windowRules.isEmpty())
    {
        qInfo() << "[LayoutManager] Keine window_flag_rules gefunden – erstelle Defaults.";
        generateDefaultWindowFlagRules();
        m_windowRules = m_backend.loadWindowFlagRules();
    }
}

void LayoutManager::reloadControlFlagRules()
{
    m_controlRules = m_backend.loadControlFlagRules();
    m_controlRulesLoaded = true;

    if (m_controlRules.isEmpty())
    {
        qInfo() << "[LayoutManager] Keine control_flag_rules gefunden – erstelle Defaults.";
        generateDefaultControlFlagRules();
        m_controlRules = m_backend.loadControlFlagRules();
    }
}

void LayoutManager::rebuildValidFlagCache()
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
