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

    // ----------------------------------------------------
    // 🪟 Window Flags (werden links-shifted)
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
    // 🎛️ Control Flags (ebenfalls links-shifted)
    // ----------------------------------------------------
    for (auto it = ctrlObj.constBegin(); it != ctrlObj.constEnd(); ++it)
    {
        QString val = it.value().toString().trimmed().toUpper();
        if (val.startsWith("0X")) val.remove(0, 2);
        if (val.endsWith("L")) val.chop(1);

        bool ok = false;
        quint32 raw = val.toUInt(&ok, 16);
        if (ok)
            m_controlFlags[it.key()] = (raw << 16);
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

        auto win = std::make_shared<WindowData>();
        win->name = it.key();

        int i = 0;

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

    qInfo() << "[LayoutManager] Validierung abgeschlossen.";
}

// -------------------------------------------------------------
// Validate Window Flags
// -------------------------------------------------------------
void LayoutManager::validateWindowFlags(WindowData* win)
{
    if (!win) return;

    const quint32 parsedWin = win->flagsMask;
    if (m_windowFlags.isEmpty()) {
        qWarning() << "[LayoutManager] Keine Window-Flags geladen!";
        return;
    }

    QStringList matchedWindowFlags;
    quint32 knownWindowBits = 0;

    for (auto it = m_windowFlags.constBegin(); it != m_windowFlags.constEnd(); ++it)
    {
        const quint32 shiftedFlag = it.value();
        knownWindowBits |= shiftedFlag;

        if ((parsedWin & shiftedFlag) == shiftedFlag)
            matchedWindowFlags << it.key();
    }

    const quint32 unknownBits = parsedWin & ~knownWindowBits;
    win->valid = (unknownBits == 0);
    win->resolvedMask = matchedWindowFlags;
    win->flagsMask = parsedWin;

    if (!win->valid)
        qWarning().noquote() << "[LayoutManager] Ungültige Fensterbits:"
                             << QString("0x%1 → unbekannt: 0x%2")
                                    .arg(QString::number(parsedWin, 16).toUpper())
                                    .arg(QString::number(unknownBits, 16).toUpper());
}

// -------------------------------------------------------------
// Validate Control Flags
// -------------------------------------------------------------
void LayoutManager::validateControlFlags(ControlData* ctrl)
{
    if (!ctrl) return;

    const quint32 parsedCtrlMask = ctrl->flagsMask;
    if (m_controlFlags.isEmpty()) {
        qWarning() << "[LayoutManager] Keine Control-Flags geladen!";
        return;
    }

    QStringList matchedControlFlags;
    quint32 knownBits = 0;

    for (auto it = m_controlFlags.constBegin(); it != m_controlFlags.constEnd(); ++it)
    {
        const quint32 shifted = it.value();
        knownBits |= shifted;
        if ((parsedCtrlMask & shifted) == shifted)
            matchedControlFlags << it.key();
    }

    const quint32 unknownBits = parsedCtrlMask & ~knownBits;
    ctrl->valid = (unknownBits == 0);
    ctrl->resolvedMask = matchedControlFlags;
    ctrl->flagsMask = parsedCtrlMask;
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


// -------------------------------------------------------------
// Generate Default Window Flag Rules
// -------------------------------------------------------------
void LayoutManager::generateDefaultWindowFlagRules()
{
    QJsonObject root;
    QJsonObject defaults;
    QJsonArray valid = {
        "WBS_NOFOCUS", "WBS_CAPTION", "WBS_NOCLING", "WBS_CHILDFRAME",
        "WBS_MOVE", "WBS_POPUP", "WBS_MANAGER", "WBS_VSCROLL", "WBS_HSCROLL",
        "WBS_SOUND", "WBS_CHILD", "WBS_DROPICON", "WBS_MODAL",
        "WBS_NODRAWFRAME", "WBS_KEY", "WBS_DOCKING", "WBS_NOFRAME", "WBS_TOPMOST"
    };
    defaults["valid"] = valid;
    root["Default"] = defaults;

    // Der Backend kennt den Speicherort selbst
    m_backend.saveWindowFlagRules(root);

    qInfo() << "[LayoutManager] Default window_flag_rules.json generiert.";
}

// -------------------------------------------------------------
// Generate Default Control Flag Rules
// -------------------------------------------------------------
void LayoutManager::generateDefaultControlFlagRules()
{
    QJsonObject root;
    root["Default"] = QJsonObject{
        {"valid", QJsonArray{"EBS_LEFT", "EBS_CENTER", "EBS_RIGHT"}},
        {"exclusive", QJsonObject{{"EBS_LEFT", QJsonArray{"EBS_CENTER", "EBS_RIGHT"}}}}
    };

    m_backend.saveControlFlagRules(root);

    qInfo() << "[LayoutManager] Default control_flag_rules.json generiert.";
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
