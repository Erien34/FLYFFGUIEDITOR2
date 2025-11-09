#include "DefineBackend.h"
#include "model/TokenData.h"
#include "EncodingUtils.h"
#include "DefineManager.h"
#include <QDebug>

bool DefineBackend::load(const QString& path, DefineManager& mgr)
{
    QFile file;
    QTextStream in;
    if (!EncodingUtils::openTextStream(file, in, path)) {
        qWarning() << "[DefineBackend] Konnte Datei nicht öffnen:" << path;
        return false;
    }

    // Lies jede Zeile und gib sie dem Manager zur Auswertung
    mgr.clear();

    while (!in.atEnd()) {
        const QString line = in.readLine();
        mgr.processDefineLine(line);
    }

    qInfo() << "[DefineBackend] Datei geladen:" << path;
    return true;
}

bool DefineBackend::saveDefines(const QString& path, const DefineManager& mgr) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "[DefineBackend] Konnte Datei nicht öffnen zum Schreiben:" << path;
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const auto& defines = mgr.allDefines();
    for (auto it = defines.cbegin(); it != defines.cend(); ++it)
        out << "#define " << it.key() << " 0x"
            << QString::number(it.value(), 16).toUpper() << "\n";

    qInfo() << "[DefineBackend] Datei gespeichert:" << path;
    return true;
}
