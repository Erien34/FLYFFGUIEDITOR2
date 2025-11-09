#pragma once
#include <QFile>
#include <QTextStream>
#include <QByteArray>
#include <QStringConverter>
#include <QDebug>

namespace EncodingUtils {

inline bool openTextStream(QFile &file, QTextStream &stream, const QString &path)
{
    file.setFileName(path);

    if (!file.exists()) {
        qWarning() << "[EncodingUtils] Datei nicht gefunden:" << path;
        return false;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[EncodingUtils] Datei konnte nicht geöffnet werden:" << path;
        return false;
    }

    QByteArray head = file.peek(4);
    if (head.startsWith("\xFF\xFE")) {
        stream.setEncoding(QStringConverter::Utf16);
        qInfo() << "[EncodingUtils] UTF-16 LE erkannt:" << path;
    } else if (head.startsWith("\xFE\xFF")) {
        stream.setEncoding(QStringConverter::Utf16BE);
        qInfo() << "[EncodingUtils] UTF-16 BE erkannt:" << path;
    } else if (head.startsWith("\xEF\xBB\xBF")) {
        stream.setEncoding(QStringConverter::Utf8);
        qInfo() << "[EncodingUtils] UTF-8 BOM erkannt:" << path;
    } else {
        stream.setEncoding(QStringConverter::Utf8);
        qInfo() << "[EncodingUtils] UTF-8 (kein BOM) angenommen:" << path;
    }

    stream.setDevice(&file);

    return true;
}

} // namespace EncodingUtils
