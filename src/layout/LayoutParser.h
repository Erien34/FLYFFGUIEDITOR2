#pragma once
#include <QObject>
#include <QString>
#include "model/TokenData.h"

// ------------------------------------------------------------
// LayoutParser
// ------------------------------------------------------------
// Liest die Datei resdata.inc, zerlegt sie in Tokens
// und speichert sie global in TokenData.
// Keine Interpretation, kein Layout- oder Textwissen.
// ------------------------------------------------------------
class LayoutParser : public QObject
{
    Q_OBJECT
public:
    explicit LayoutParser(QObject* parent = nullptr);

    // Datei oder Text parsen
    bool parse(const QString& path);
    bool parseText(const QString& text);

signals:
    // Signal: neue Tokens verfügbar
    void tokensReady();

private:
    void tokenize(const QString& text);
    static QString unquote(const QString& s);
};

