#include "TextBackend.h"
#include "TextManager.h"
#include "EncodingUtils.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

// ------------------------------------------------------------
// Lädt textClient.txt (reine Textdaten)
// ------------------------------------------------------------
bool TextBackend::loadText(const QString& path, TextManager& mgr)
{
    QFile file;
    QTextStream in;
    if (!EncodingUtils::openTextStream(file, in, path)) {
        qWarning() << "[TextBackend] Konnte Textdatei nicht öffnen:" << path;
        return false;
    }

    mgr.clear();

    while (!in.atEnd()) {
        const QString line = in.readLine();
        mgr.processTextLine(line);
    }

    qInfo() << "[TextBackend] textClient.txt geladen:" << path;
    return true;
}

// ------------------------------------------------------------
// Lädt textClient.inc (Gruppenstruktur)
// ------------------------------------------------------------
bool TextBackend::loadInc(const QString& path, TextManager& mgr)
{
    QFile file;
    QTextStream in;
    if (!EncodingUtils::openTextStream(file, in, path)) {
        qWarning() << "[TextBackend] Konnte INC-Datei nicht öffnen:" << path;
        return false;
    }

    mgr.clearIncState(); // optional: interne Gruppen zurücksetzen

    while (!in.atEnd()) {
        const QString line = in.readLine();
        mgr.processIncLine(line);
    }

    qInfo() << "[TextBackend] textClient.inc geladen:" << path;
    return true;
}

// ------------------------------------------------------------
// Speichert textClient.txt
// ------------------------------------------------------------
bool TextBackend::saveText(const QString& path, const TextManager& mgr) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[TextBackend] Konnte Textdatei nicht schreiben:" << path;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const auto texts = mgr.allTexts();
    for (auto it = texts.constBegin(); it != texts.constEnd(); ++it)
        out << it.key() << "\t" << it.value() << "\n";

    qInfo() << "[TextBackend] textClient.txt gespeichert:" << path;
    return true;
}

// ------------------------------------------------------------
// Speichert textClient.inc
// ------------------------------------------------------------
bool TextBackend::saveInc(const QString& path, const TextManager& mgr) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[TextBackend] Konnte INC-Datei nicht schreiben:" << path;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const auto groups = mgr.allGroups();
    for (const QString& tid : groups) {
        const auto ids = mgr.idsForGroup(tid);
        out << tid << " 0xffffffff\n{\n";
        for (const QString& id : ids)
            out << "    " << id << "\n";
        out << "}\n\n";
    }

    qInfo() << "[TextBackend] textClient.inc gespeichert:" << path;
    return true;
}


