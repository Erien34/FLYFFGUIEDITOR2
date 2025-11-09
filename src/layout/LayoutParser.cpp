#include "LayoutParser.h"
#include "EncodingUtils.h"
#include "model/TokenData.h"
#include <QDebug>
#include <QRegularExpression>

LayoutParser::LayoutParser(QObject* parent)
    : QObject(parent)
{
}

// Entfernt Anführungszeichen
QString LayoutParser::unquote(const QString& s)
{
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
        return s.mid(1, s.size() - 2);
    return s;
}

// Liest Datei über EncodingUtils
bool LayoutParser::parse(const QString& path)
{
    QFile file;
    QTextStream in;
    if (!EncodingUtils::openTextStream(file, in, path)) {
        qWarning() << "[LayoutParser] Datei konnte nicht geöffnet werden:" << path;
        return false;
    }

    QString text = in.readAll();
    file.close();
    return parseText(text);
}

// Tokenisierung aus Text
bool LayoutParser::parseText(const QString& text)
{
    qInfo() << "[LayoutParser] Tokenisierung gestartet...";

    TokenData::instance().clear();
    tokenize(text);

    qInfo() << "[LayoutParser] Tokenisierung abgeschlossen. Tokens:"
            << TokenData::instance().all().size();

    emit tokensReady();
    return true;
}

// Tokenisierung
void LayoutParser::tokenize(const QString& text)
{
    QStringList lines = text.split('\n');
    QString currentWindow;
    QString lastComment;
    int order = 0;

    QMap<QString, QList<Token>> tokenMap;
    QList<Token> currentTokens;

    for (QString rawLine : lines)
    {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        Token t;
        t.value = line;
        t.orderIndex = order++;
        t.comment.clear();
        t.windowName = currentWindow;

        // --- Klassifizierung ---
        if (line.startsWith("//")) {
            // Kommentar → speichern, kann semantisch wichtig sein
            t.type = "Comment";
            lastComment = line.mid(2).trimmed();
            t.comment = lastComment;
        }
        else if (line == "{" || line == "}") {
            // Strukturklammer → Other
            t.type = "Other";
        }
        else if (line.startsWith("APP_") || line.startsWith("WND_") ||
                 line.startsWith("DPS_") || line.startsWith("CONFIRM_")) {
            // Fensterheader
            t.type = "WindowHeader";
            currentWindow = line.section(' ', 0, 0);
        }
        else if (line.startsWith("WTYPE_")) {
            // Controlheader
            t.type = "ControlHeader";
            t.controlId = line.section(' ', 1, 1);
        }
        else if (line.startsWith("IDS_RESDATA_INC_") || line.startsWith("IDS_")) {
            // Textzeilen
            t.type = "Text";
        }
        else {
            // Alles andere
            t.type = "Other";
        }

        // --- Speichern ---
        // „Other“-Tokens (Klammern) brauchen wir nicht persistent speichern
        if (t.type != "Other")
            tokenMap[currentWindow].append(t);
    }

    // Globale Speicherung
    for (auto it = tokenMap.cbegin(); it != tokenMap.cend(); ++it)
        TokenData::instance().addTokens(it.key(), it.value());
}
