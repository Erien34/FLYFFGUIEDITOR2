#include "TextManager.h"
#include <QRegularExpression>
#include <QDebug>

// ------------------------------------------------------------
// Clear
// ------------------------------------------------------------

void TextManager::clear()
{
    m_texts.clear();
    m_groups.clear();
    m_idToGroup.clear();
}

void TextManager::clearIncState()
{
    m_groups.clear();
    m_idToGroup.clear();
}

// ------------------------------------------------------------
// textClient.txt verarbeiten (IDS → Text)
// ------------------------------------------------------------
void TextManager::processTextLine(const QString& line)
{
    if (line.isEmpty() || line.startsWith("//"))
        return;

    const QString trimmed = line.trimmed();
    if (!trimmed.startsWith("IDS_"))
        return;

    const QStringList parts = trimmed.split(QRegularExpression("\\s+"));
    if (parts.size() < 2)
        return;

    const QString key = parts[0];
    const QString value = parts.mid(1).join(" ");
    m_texts[key] = value;
    setDirty();
}

// ------------------------------------------------------------
// textClient.inc verarbeiten (TID → IDS)
// ------------------------------------------------------------
void TextManager::processIncLine(const QString& line)
{
    QString trimmed = line.trimmed();
    if (trimmed.isEmpty() || trimmed.startsWith("//"))
        return;

    static QRegularExpression tidRegex(R"(^\s*(TID_[A-Za-z0-9_]+))");
    static QRegularExpression idsRegex(R"(^\s*(IDS_[A-Za-z0-9_]+))");

    if (trimmed.startsWith("TID_")) {
        QRegularExpressionMatch m = tidRegex.match(trimmed);
        if (m.hasMatch()) {
            m_currentTid = m.captured(1);
            addGroup(m_currentTid);
        }
        return;
    }

    if (trimmed.startsWith("IDS_") && !m_currentTid.isEmpty()) {
        QRegularExpressionMatch m = idsRegex.match(trimmed);
        if (m.hasMatch()) {
            addIdToGroup(m_currentTid, m.captured(1));
        }
    }
    setDirty();
}


// ------------------------------------------------------------
// Token-Rebuild (globaler Aufbau aus Tokens)
// ------------------------------------------------------------
void TextManager::rebuildFromTokens(const QList<Token>& tokens)
{
    qInfo() << "[TextManager] Rebuild from Tokens gestartet (Tokens:" << tokens.size() << ")";

    m_groups.clear();
    m_idToGroup.clear();

    QString currentTid;
    int countGroups = 0;
    int countIds = 0;

    for (const Token& t : tokens) {
        // Nur Tokens vom Typ "Text" oder "WindowHeader" berücksichtigen
        if (t.type == "Text") {
            // Fenstergruppe bestimmen
            QString tid = t.windowName.isEmpty()
                              ? QStringLiteral("TID_UNASSIGNED")
                              : "TID_" + t.windowName.toUpper();

            // Control-ID bestimmen
            QString id = t.controlId.isEmpty()
                             ? QStringLiteral("IDS_UNNAMED")
                             : "IDS_" + t.controlId.toUpper();

            addGroup(tid);
            addIdToGroup(tid, id);

            // Textwert übernehmen (falls vorhanden)
            if (!t.value.isEmpty() && !m_texts.contains(id)) {
                m_texts[id] = t.value;
            }

            ++countIds;
        }
        else if (t.type == "WindowHeader") {
            // Fenster erzeugt eigene TID-Gruppe
            QString tid = "TID_" + t.windowName.toUpper();
            addGroup(tid);
            ++countGroups;
        }
    }

    qInfo() << "[TextManager] Rebuild abgeschlossen:" << countGroups << "Gruppen," << countIds << "Texte";
}

// ------------------------------------------------------------
// Gruppenaufbau
// ------------------------------------------------------------
void TextManager::addGroup(const QString& tid)
{
    if (!m_groups.contains(tid))
        m_groups[tid] = TextGroup{ tid, {} };
    setDirty();
}

void TextManager::addIdToGroup(const QString& tid, const QString& id)
{
    if (!m_groups.contains(tid))
        m_groups[tid] = TextGroup{ tid, {} };

    m_groups[tid].ids.append(id);
    m_idToGroup[id] = tid;
    setDirty();
}

// ------------------------------------------------------------
// Zugriffsfunktionen
// ------------------------------------------------------------
QString TextManager::value(const QString& id) const
{
    return m_texts.value(id);
}

QString TextManager::groupForId(const QString& id) const
{
    return m_idToGroup.value(id);
}

QList<QString> TextManager::idsForGroup(const QString& tid) const
{
    if (!m_groups.contains(tid))
        return {};
    return m_groups[tid].ids;
}

QStringList TextManager::allGroups() const
{
    return m_groups.keys();
}
