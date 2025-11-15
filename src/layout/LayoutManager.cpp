#include "LayoutManager.h"

#include "LayoutBackend.h"
#include "behavior/BehaviorManager.h"
#include "model/TokenData.h"   // ggf. Pfad anpassen
#include "model/WindowData.h"
#include "model/ControlData.h"

#include <QDebug>
#include <QRegularExpression>

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
// Konstruktor
// -------------------------------------------------------------
LayoutManager::LayoutManager(LayoutParser& parser, LayoutBackend& backend)
    : m_parser(parser)
    , m_backend(backend)
{
    connect(&m_parser, &LayoutParser::tokensReady,
            this, &LayoutManager::tokensReady);
}

// -------------------------------------------------------------
// Parserdaten übernehmen – Controls vollständig nach ControlData mappen
// -------------------------------------------------------------
void LayoutManager::refreshFromParser()
{
    m_windows.clear();

    const auto tokenMap = TokenData::instance().all();

    for (auto it = tokenMap.cbegin(); it != tokenMap.cend(); ++it)
    {
        const QString       windowName = it.key();
        const QList<Token>& tokens     = it.value();

        if (tokens.isEmpty())
            continue;
        if (windowName.trimmed().isEmpty())
            continue;

        auto win  = std::make_shared<WindowData>();
        win->name = windowName;

        int i = 0;

        // =========================================================
        // Window-Header parsen
        // =========================================================
        while (i < tokens.size() && tokens[i].type != "WindowHeader")
            ++i;

        if (i < tokens.size() && tokens[i].type == "WindowHeader")
        {
            const QStringList p = tokens[i].value.split(
                QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            ++i;

            // grob: 0: name/typ, 1: texture, 2: title, 3: modus,
            //       4: width, 5: height, 6: flagsHex, 7: mod
            if (p.size() >= 8)
            {
                win->texture   = unquote(p[1]);
                win->titletext = p[2];
                win->modus     = p[3].toInt();
                win->width     = p[4].toInt();
                win->height    = p[5].toInt();
                win->flagsHex  = p[6];
                win->mod       = p[7].toInt();

                bool ok = false;
                QString clean = win->flagsHex.trimmed().toUpper();
                if (clean.startsWith("0X")) clean.remove(0, 2);
                if (clean.endsWith("L"))   clean.chop(1);

                win->flagsMask = clean.toUInt(&ok, 16);

                if (!ok) {
                    win->flagsMask = 0;
                    qWarning().noquote()
                        << "[LayoutManager] Ungültiger Window-Flagwert:"
                        << win->flagsHex << "bei" << win->name;
                }

                // -------------------------------------------------
                // 🛠 AUTO-FIX: kaputte Fensterflags hochschiften
                // -------------------------------------------------
                if (win->flagsMask > 0 && win->flagsMask < 0x10000)
                {
                    qWarning().noquote()
                    << "[LayoutManager] Auto-Fix → Window" << win->name
                    << "hat LOW-Flag 0x" + QString::number(win->flagsMask,16)
                    << "→ shift nach HIGH.";

                    win->flagsMask <<= 16;
                }
            }
        }

        // Window-Texte überspringen (werden beim Serialisieren direkt aus Tokens gelesen)
        while (i < tokens.size() && tokens[i].type != "ControlHeader")
            ++i;

        // =========================================================
        // Controls einlesen
        // =========================================================
        while (i < tokens.size())
        {
            if (tokens[i].type != "ControlHeader")
            {
                ++i;
                continue;
            }

            const Token& headerTok = tokens[i];
            auto ctrl              = std::make_shared<ControlData>();

            ctrl->rawHeader = headerTok.value;
            ctrl->tokens    = headerTok.value.split(
                QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            const QStringList& p = ctrl->tokens;

            // Struktur:
            // 0: type
            // 1: id
            // 2: texture
            // 3: mod0
            // 4-7: x1 y1 x2 y2
            // 8: flagsHex
            // 9-12: mod1..mod4
            // 13-15: ggf. Farbe (RGB oder packed)

            if (p.size() >= 1) ctrl->type    = p[0];
            if (p.size() >= 2) ctrl->id      = p[1];
            if (p.size() >= 3) ctrl->texture = unquote(p[2]);
            if (p.size() >= 4) ctrl->mod0    = p[3].toInt();

            if (p.size() >= 8)
            {
                ctrl->x1 = p[4].toInt();
                ctrl->y1 = p[5].toInt();
                ctrl->x2 = p[6].toInt();
                ctrl->y2 = p[7].toInt();
            }

            if (p.size() >= 9)
            {
                ctrl->flagsHex = p[8];

                bool ok = false;
                QString clean = ctrl->flagsHex.trimmed().toUpper();

                // Präfixe entfernen
                if (clean.startsWith("0X"))
                    clean.remove(0, 2);
                if (clean.endsWith("L"))
                    clean.chop(1);

                // In uint32 parsen
                ctrl->flagsMask = clean.toUInt(&ok, 16);
                if (!ok)
                {
                    ctrl->flagsMask = 0;
                }

                // --- Flags zerlegen ---
                ctrl->lowFlags  =  ctrl->flagsMask        & 0x0000FFFF;
                ctrl->midFlags  = (ctrl->flagsMask >> 16) & 0x000000FF;
                ctrl->highFlags = (ctrl->flagsMask >> 24) & 0x000000FF;
            }

            if (p.size() >= 10) ctrl->mod1 = p[9].toInt();
            if (p.size() >= 11) ctrl->mod2 = p[10].toInt();
            if (p.size() >= 12) ctrl->mod3 = p[11].toInt();
            if (p.size() >= 13) ctrl->mod4 = p[12].toInt();

            // --- Farbe ---
            if (p.size() >= 16)
            {
                bool okR = false, okG = false, okB = false;
                int v1   = p[13].toInt(&okR);
                int v2   = p[14].toInt(&okG);
                int v3   = p[15].toInt(&okB);

                if (okR && okG && okB &&
                    v1 >= 0 && v1 <= 255 &&
                    v2 >= 0 && v2 <= 255 &&
                    v3 >= 0 && v3 <= 255)
                {
                    ctrl->color = QColor(v1, v2, v3);
                }
                else
                {
                    bool okPacked = false;
                    int packed    = p[13].toInt(&okPacked);
                    if (okPacked)
                    {
                        quint32 u = static_cast<quint32>(packed);
                        int r     = (u >> 16) & 0xFF;
                        int g     = (u >> 8)  & 0xFF;
                        int b     =  u        & 0xFF;
                        ctrl->color = QColor(r, g, b);
                    }
                    else
                    {
                        ctrl->color = QColor(255, 255, 255);
                    }
                }
            }
            else
            {
                ctrl->color = QColor(255, 255, 255);
            }

            // nachfolgende Text-Tokens → Title / Tooltip
            ++i; // hinter den Header
            QString ctrlTitleId;
            QString ctrlTooltipId;
            int tcount = 0;

            while (i < tokens.size() && tokens[i].type != "ControlHeader")
            {
                if (tokens[i].type == "Text")
                {
                    if (tcount == 0)
                        ctrlTitleId = tokens[i].value.trimmed();
                    else if (tcount == 1)
                        ctrlTooltipId = tokens[i].value.trimmed();
                    ++tcount;
                }
                ++i;
            }

            ctrl->titleId   = ctrlTitleId;
            ctrl->tooltipId = ctrlTooltipId;

            win->controls.push_back(ctrl);
        }

        m_windows.push_back(win);
    }

    qInfo().noquote()
        << QString("[LayoutManager] Parserdaten übernommen → %1 Fenster.")
               .arg(m_windows.size());
}


// -------------------------------------------------------------
// Layout verarbeiten (ruft BehaviorManager)
// -------------------------------------------------------------
void LayoutManager::processLayout()
{
    qInfo() << "[LayoutManager] Verarbeite Layouts...";

    if (!m_behaviorManager)
    {
        qWarning() << "[LayoutManager] Kein BehaviorManager zugewiesen!";
        return;
    }

    for (auto& wndPtr : m_windows)
    {
        if (!wndPtr) continue;

        // 1) Window-Flags validieren
        m_behaviorManager->validateWindowFlags(wndPtr.get());

        // 2) BehaviorInfo für Fenster erzeugen
        wndPtr->behavior = m_behaviorManager->resolveBehavior(*wndPtr);

        // 3) Controls
        for (auto& ctrlPtr : wndPtr->controls)
        {
            if (!ctrlPtr) continue;

            m_behaviorManager->validateControlFlags(ctrlPtr.get());
            ctrlPtr->behavior = m_behaviorManager->resolveBehavior(*ctrlPtr);
        }
    }

    // Nachgelagerte Analysen
    m_behaviorManager->analyzeControlTypes(m_windows);
    m_behaviorManager->generateUnknownControls(m_windows);

    qInfo() << "[LayoutManager] Validierung & Behavior-Zuordnung abgeschlossen.";
}

// -------------------------------------------------------------
// Layout serialisieren
// -------------------------------------------------------------
QString LayoutManager::serializeLayout() const
{
    QString out;
    out.reserve(131072);

    const auto& tokenMap = TokenData::instance().all();

    for (auto it = tokenMap.cbegin(); it != tokenMap.cend(); ++it)
    {
        const QString       windowName = it.key();
        const QList<Token>& tokens     = it.value();

        if (tokens.isEmpty())
            continue;

        // passendes WindowData suchen
        std::shared_ptr<WindowData> winData;
        for (const auto& w : m_windows)
        {
            if (w && w->name.compare(windowName, Qt::CaseInsensitive) == 0)
            {
                winData = w;
                break;
            }
        }

        int i = 0;

        // WindowHeader schreiben
        while (i < tokens.size() && tokens[i].type != "WindowHeader")
            ++i;
        if (i >= tokens.size())
            continue;

        const QString wndHeader = tokens[i++].value.trimmed();
        out += wndHeader + "\r\n";

        // Window-Texte (Title/Help)
        QString titleId;
        QString helpId;
        int textCount = 0;

        int j = i;
        while (j < tokens.size() && tokens[j].type != "ControlHeader")
        {
            if (tokens[j].type == "Text")
            {
                if (textCount == 0)
                    titleId = tokens[j].value.trimmed();
                else if (textCount == 1)
                    helpId = tokens[j].value.trimmed();
                ++textCount;
            }
            ++j;
        }
        i = j;

        out += "{\r\n    // Title String\r\n";
        if (!titleId.isEmpty())
            out += "    " + titleId + "\r\n";
        out += "}\r\n";

        out += "{\r\n    // Help Key\r\n";
        if (!helpId.isEmpty())
            out += "    " + helpId + "\r\n";
        out += "}\r\n";

        // Controls
        out += "{\r\n";

        int controlIndex = 0;

        while (i < tokens.size())
        {
            if (tokens[i].type != "ControlHeader")
            {
                ++i;
                continue;
            }

            const QString rawHeader = tokens[i++].value.trimmed();
            QStringList parts = rawHeader.split(
                QRegularExpression("\\s+"), Qt::SkipEmptyParts);

            std::shared_ptr<ControlData> ctrlData;
            if (winData && controlIndex < winData->controls.size())
            {
                ctrlData = winData->controls[controlIndex];
                ++controlIndex;
            }

            // Falls wir eine dekodierte Farbe haben: RGB in Header schreiben
            if (ctrlData && ctrlData->color.isValid())
            {
                const QColor& c = ctrlData->color;
                QString r = QString::number(c.red());
                QString g = QString::number(c.green());
                QString b = QString::number(c.blue());

                if (parts.size() >= 16)
                {
                    parts[13] = r;
                    parts[14] = g;
                    parts[15] = b;
                }
                else if (parts.size() >= 13)
                {
                    parts << r << g << b;
                }
                else
                {
                    parts << r << g << b;
                }
            }

            out += "    " + parts.join(" ") + "\r\n";

            // nachfolgende Text-Tokens → Control-Title / Tooltip
            QString ctrlTitleId;
            QString ctrlTooltipId;
            int tcount = 0;

            while (i < tokens.size() && tokens[i].type != "ControlHeader")
            {
                if (tokens[i].type == "Text")
                {
                    if (tcount == 0)
                        ctrlTitleId = tokens[i].value.trimmed();
                    else if (tcount == 1)
                        ctrlTooltipId = tokens[i].value.trimmed();
                    ++tcount;
                }
                ++i;
            }

            out += "    {\r\n        // Title String\r\n";
            if (!ctrlTitleId.isEmpty())
                out += "        " + ctrlTitleId + "\r\n";
            out += "    }\r\n";

            out += "    {\r\n        // ToolTip\r\n";
            if (!ctrlTooltipId.isEmpty())
                out += "        " + ctrlTooltipId + "\r\n";
            out += "    }\r\n";
        }

        out += "}\r\n\r\n";
    }

    return out;
}

// -------------------------------------------------------------
// Fenster finden
// -------------------------------------------------------------
std::shared_ptr<WindowData> LayoutManager::findWindow(const QString& name) const
{
    for (const auto& wnd : m_windows)
    {
        if (wnd && wnd->name.compare(name, Qt::CaseInsensitive) == 0)
            return wnd;
    }
    return nullptr;
}
