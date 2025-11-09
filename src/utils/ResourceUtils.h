#pragma once
#include <QMap>
#include <QPixmap>
#include <QImage>
#include <QFile>
#include <QDirIterator>
#include <QImageReader>
#include <QDebug>
#include <QTextStream>
#include <QDir>
#include <QIcon>

namespace ResourceUtils
{

// ------------------------------------------------------------
// 🔹 FlyFF-kompatibler TGA-Loader (unkomprimiert, 24/32-Bit)
// ------------------------------------------------------------
inline QImage loadFlyffTga(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[ResourceUtils] Konnte TGA-Datei nicht öffnen:" << path;
        return QImage();
    }

    QByteArray data = file.readAll();
    if (data.size() < 18)
        return QImage();

    const unsigned char* d = reinterpret_cast<const unsigned char*>(data.constData());
    int width  = d[12] + (d[13] << 8);
    int height = d[14] + (d[15] << 8);
    int bpp    = d[16];

    if (width <= 0 || height <= 0 || (bpp != 24 && bpp != 32))
        return QImage();

    int bytesPerPixel = bpp / 8;
    int imageSize = width * height * bytesPerPixel;

    if (data.size() < 18 + imageSize) {
        qWarning() << "[ResourceUtils] TGA-Datei unvollständig:" << path;
        return QImage();
    }

    QImage img(width, height, QImage::Format_ARGB32);
    const unsigned char* src = d + 18;

    // Vertikal flippen, Farbreihenfolge korrigieren (BGR → RGB)
    for (int y = 0; y < height; ++y) {
        QRgb* dest = reinterpret_cast<QRgb*>(img.scanLine(height - 1 - y));
        for (int x = 0; x < width; ++x) {
            uchar b = *src++;
            uchar g = *src++;
            uchar r = *src++;
            uchar a = (bytesPerPixel == 4) ? *src++ : 255;
            dest[x] = qRgba(r, g, b, a);
        }
    }

    return img;
}

// ------------------------------------------------------------
// 🔹 Entfernt FlyFF-typische Magenta-Maskenfarbe (255, 0, 255)
// ------------------------------------------------------------
inline QPixmap applyMagentaMask(const QPixmap& src)
{
    if (src.isNull())
        return src;

    QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);

    const int w = img.width();
    const int h = img.height();

    for (int y = 0; y < h; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < w; ++x) {
            QRgb px = line[x];
            if (qRed(px) == 255 && qGreen(px) == 0 && qBlue(px) == 255) {
                line[x] = qRgba(0, 0, 0, 0); // transparent setzen
            }
        }
    }

    return QPixmap::fromImage(img);
}

// ------------------------------------------------------------
// 🔹 Entfernt transparente Ränder / clamped sie an benachbarte Pixel
// ------------------------------------------------------------
inline QPixmap clampTransparentEdges(const QPixmap& src)
{
    if (src.isNull() || !src.hasAlphaChannel())
        return src;

    QImage img = src.toImage().convertToFormat(QImage::Format_ARGB32);
    const int w = img.width();
    const int h = img.height();

    auto getSafe = [&](int x, int y) -> QRgb {
        x = std::clamp(x, 0, w - 1);
        y = std::clamp(y, 0, h - 1);
        return img.pixel(x, y);
    };

    // Linker / rechter Rand
    for (int y = 0; y < h; ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(img.scanLine(y));
        if (qAlpha(line[0]) < 255 && w > 1)
            line[0] = getSafe(1, y);
        if (qAlpha(line[w - 1]) < 255 && w > 1)
            line[w - 1] = getSafe(w - 2, y);
    }

    // Oberer / unterer Rand
    for (int x = 0; x < w; ++x) {
        if (qAlpha(img.pixel(x, 0)) < 255 && h > 1)
            img.setPixel(x, 0, getSafe(x, 1));
        if (qAlpha(img.pixel(x, h - 1)) < 255 && h > 1)
            img.setPixel(x, h - 1, getSafe(x, h - 2));
    }

    return QPixmap::fromImage(img);
}

// ------------------------------------------------------------
// 🔹 Helper: Lädt eine einzelne Pixmap (mit TGA-Fallback)
// ------------------------------------------------------------
inline QPixmap loadSinglePixmap(const QString& filePath)
{
    QFileInfo fi(filePath);
    QPixmap pix;

    if (fi.suffix().compare("tga", Qt::CaseInsensitive) == 0) {
        QImage tga = loadFlyffTga(filePath);
        if (!tga.isNull())
            pix = QPixmap::fromImage(tga);
    } else {
        QImageReader reader(filePath);
        reader.setAutoTransform(true);
        pix = QPixmap::fromImageReader(&reader);
    }

    return pix;
}

// ------------------------------------------------------------
// 🔹 Lädt alle Pixmaps (Themes / Fensterrahmen / UI)
// ------------------------------------------------------------
inline QMap<QString, QPixmap> loadPixmaps(const QString& dirPath)
{
    QMap<QString, QPixmap> result;

    if (dirPath.isEmpty() || !QDir(dirPath).exists()) {
        qWarning() << "[ResourceUtils] Theme-Pfad ungültig:" << dirPath;
        return result;
    }

    const QStringList filters = {"*.tga", "*.png", "*.jpg", "*.bmp"};
    QDirIterator it(dirPath, filters, QDir::Files, QDirIterator::Subdirectories);

    QFile logFile(dirPath + "/theme_debug_log.txt");
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream log(&logFile);

    int loaded = 0;
    int failed = 0;

    while (it.hasNext()) {
        const QString filePath = it.next();
        QFileInfo fi(filePath);
        const QString key = fi.baseName().toLower();

        QPixmap pix = loadSinglePixmap(filePath);
        pix = clampTransparentEdges(pix);
        pix = applyMagentaMask(pix);     // 🆕 FlyFF-Magenta-Transparenz anwenden

        if (pix.isNull()) {
            log << "❌ Fehler: " << filePath << "\n";
            failed++;
            continue;
        }

        result.insert(key, pix);
        loaded++;
        log << "✅ Geladen: " << key
            << " (" << pix.width() << "x" << pix.height() << ")\n";
    }

    log << "\nGesamt geladen: " << loaded
        << " | Fehler: " << failed
        << " | Gesamtdateien: " << (loaded + failed) << "\n";

    logFile.close();

    qInfo().noquote() << QString("[ResourceUtils] Themes geladen: %1 Texturen (%2 Fehler)")
                             .arg(loaded)
                             .arg(failed);
    qInfo().noquote() << QString("[ResourceUtils] Log-Datei: %1").arg(logFile.fileName());

    return result;
}

// ------------------------------------------------------------
// 🔹 Lädt Icons (kleinere Grafiken, Buttons etc.)
// ------------------------------------------------------------
inline QMap<QString, QIcon> loadIcons(const QString& dirPath)
{
    QMap<QString, QIcon> result;

    if (dirPath.isEmpty() || !QDir(dirPath).exists()) {
        qWarning() << "[ResourceUtils] Icon-Pfad ungültig:" << dirPath;
        return result;
    }

    const QStringList filters = {"*.tga", "*.png", "*.jpg", "*.bmp"};
    QDirIterator it(dirPath, filters, QDir::Files, QDirIterator::Subdirectories);

    QFile logFile(dirPath + "/icon_debug_log.txt");
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream log(&logFile);

    int loaded = 0;
    int failed = 0;

    while (it.hasNext()) {
        const QString filePath = it.next();
        QFileInfo fi(filePath);
        const QString key = fi.baseName().toLower();

        // reuse: dein bestehender Loader
        QPixmap pix = loadSinglePixmap(filePath);

        if (pix.isNull()) {
            log << "❌ Fehler: " << filePath << "\n";
            failed++;
            continue;
        }

        QIcon icon;
        icon.addPixmap(pix);                // falls du später mehrere Größen hinzufügen willst
        result.insert(key, icon);

        loaded++;
        log << "✅ Geladen: " << key
            << " (" << pix.width() << "x" << pix.height() << ")\n";
    }

    log << "\nGesamt geladen: " << loaded
        << " | Fehler: " << failed
        << " | Gesamtdateien: " << (loaded + failed) << "\n";

    logFile.close();

    qInfo().noquote() << QString("[ResourceUtils] Icons geladen: %1 Dateien (%2 Fehler)")
                             .arg(loaded)
                             .arg(failed);
    qInfo().noquote() << QString("[ResourceUtils] Log-Datei: %1").arg(logFile.fileName());

    return result;
}

// DetectTilePrefix – erkennt dynamisch Fenster- und Control-Tiles
inline QString detectTilePrefix(const QMap<QString, QPixmap>& themes,
                                const QStringList& candidates = {},
                                const QString& preferred = QString())
{
    // 1️⃣ Explizit bevorzugtes Präfix prüfen
    if (!preferred.isEmpty()) {
        QString key = preferred.toLower();
        if (themes.contains(key + "00") || themes.contains(key + "01"))
            return key;
    }

    // 2️⃣ Wenn Kandidatenliste übergeben wurde → NUR diese prüfen
    QStringList prefixesToCheck;
    if (!candidates.isEmpty()) {
        prefixesToCheck = candidates;
    } else {
        prefixesToCheck = {
            "wndtile", "wndedittile", "wndbutton", "buttnormal",
            "wndcheckbox", "wndscrollbar", "wndtabtile",
            "wndbar", "wndlistbox", "wndlistctrl", "wndedit", "edit", "button"
        };
    }

    QString bestMatch;
    int bestScore = -1;

    for (const QString& prefix : prefixesToCheck) {
        int count = 0;
        for (int i = 0; i < 9; ++i) {
            QString key = QString("%1%2").arg(prefix).arg(i, 2, 10, QChar('0'));
            if (themes.contains(key))
                count++;
        }
        if (count > bestScore) {
            bestScore = count;
            bestMatch = prefix;
        }
    }

    if (!bestMatch.isEmpty()) {
        qInfo().noquote() << "[ResourceUtils] detectTilePrefix ✓ erkannt:" << bestMatch;
        return bestMatch;
    }

    // 3️⃣ Dynamischer Fallback: suche ein beliebiges Schlüssel-Muster mit 00–08 am Ende
    for (auto it = themes.constBegin(); it != themes.constEnd(); ++it) {
        QString key = it.key().toLower();
        if (key.endsWith("00") || key.endsWith("01")) {
            QString base = key.left(key.size() - 2);
            qInfo().noquote() << "[ResourceUtils] detectTilePrefix (Fallback):" << base;
            return base;
        }
    }

    qWarning() << "[ResourceUtils] detectTilePrefix ⚠ Kein Prefix gefunden.";
    return QString();
}

// ------------------------------------------------------------
// 🔹 Findet eine Textur im Theme-Map – tolerant gegenüber Pfaden / Endungen / Case
// ------------------------------------------------------------
inline QString findTextureKey(const QMap<QString, QPixmap>& themes,
                              const QString& textureName)
{
    if (textureName.isEmpty() || themes.isEmpty())
        return QString();

    QString texKey = textureName.toLower();
    if (texKey.endsWith(".tga"))
        texKey.chop(4);
    if (texKey.endsWith(".png"))
        texKey.chop(4);
    if (texKey.endsWith(".jpg"))
        texKey.chop(4);

    // Direkter Treffer?
    if (themes.contains(texKey))
        return texKey;

    // Teiltreffer suchen
    for (const auto& key : themes.keys()) {
        QString lowerKey = key.toLower();
        if (lowerKey.contains(texKey))
            return key;
    }

    qDebug() << "[ResourceUtils] findTextureKey ⚠ Kein Treffer für" << textureName;
    return QString();
}
} // namespace ResourceUtils
