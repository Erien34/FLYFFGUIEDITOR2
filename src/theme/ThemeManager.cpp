#include "ThemeManager.h"
#include "ResourceUtils.h"
#include <QDir>
#include <QDebug>
#include <QFileInfo>

ThemeManager::ThemeManager(FileManager* fileMgr, QObject* parent)
    : QObject(parent), m_fileMgr(fileMgr)
{
}

void ThemeManager::refreshFromTokens(const QList<Token>& tokens)
{
    Q_UNUSED(tokens);
    emit texturesUpdated();
}

void ThemeManager::clear()
{
    m_themes.clear();
    m_currentTheme.clear();
}

QMap<QString, QPixmap> ThemeManager::loadPixmaps(const QString& dirPath,
                                                 const QString& themeName)
{
    QMap<QString, QPixmap> result;

    if (dirPath.isEmpty() || !QDir(dirPath).exists()) {
        qWarning() << "[ThemeManager] Ungültiger Theme-Pfad:" << dirPath;
        return result;
    }

    const QStringList filters = { "*.tga", "*.png", "*.jpg", "*.bmp" };
    QDirIterator it(dirPath, filters, QDir::Files, QDirIterator::Subdirectories);

    QString logName = themeName.isEmpty()
                          ? "theme_debug_log.txt"
                          : QString("theme_debug_%1_log.txt").arg(themeName);

    QFile logFile(QDir(dirPath).filePath(logName));
    logFile.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream log(&logFile);

    int loaded = 0;
    int failed = 0;

    while (it.hasNext()) {
        QString filePath = it.next();
        QFileInfo fi(filePath);

        // 🔹 Der Key ist der Basisname in lowercase
        QString key = fi.baseName().toLower();

        QPixmap pix;

        // TGA mit Flyff-spezialbehandlung
        if (fi.suffix().compare("tga", Qt::CaseInsensitive) == 0) {
            QImage tga = ResourceUtils::loadFlyffTga(filePath);
            if (!tga.isNull())
                pix = QPixmap::fromImage(tga);
        }
        else {
            QImageReader reader(filePath);
            reader.setAutoTransform(true);
            pix = QPixmap::fromImageReader(&reader);
        }

        if (pix.isNull()) {
            log << "❌ Fehler: " << filePath << "\n";
            failed++;
            continue;
        }

        // 🔹 Flyff-typische Korrekturen
        pix = ResourceUtils::clampTransparentEdges(pix);
        pix = ResourceUtils::applyMagentaMask(pix);

        log << "✓ Geladen: " << key << " (" << pix.width() << "x" << pix.height() << ")\n";
        result.insert(key, pix);
        loaded++;
    }

    log << "\nGesamt geladen: " << loaded
        << " | Fehler: " << failed
        << " | Dateien: " << (loaded + failed) << "\n";

    logFile.close();

    qInfo().noquote() << QString("[ThemeManager] Log-Datei: %1")
                             .arg(logFile.fileName());

    return result;
}

bool ThemeManager::loadTheme(const QString& themeName)
{
    if (!m_fileMgr) {
        qWarning() << "[ThemeManager] Kein FileManager gesetzt!";
        return false;
    }

    QString defaultPath = m_fileMgr->defaultThemePath();
    QString themePath   = m_fileMgr->themeFolderPath(themeName);

    if (defaultPath.isEmpty() || !QDir(defaultPath).exists()) {
        qWarning() << "[ThemeManager] Default-Theme nicht gefunden:" << defaultPath;
        return false;
    }

    qInfo().noquote() << "[ThemeManager] Lade Theme:" << themeName;

    // 1️⃣ Default-Theme laden
    QMap<QString, QPixmap> defaultPixmaps =
        loadPixmaps(defaultPath, "Default");

    // 2️⃣ Optionales Theme (z. B. English) laden
    QMap<QString, QPixmap> themePixmaps;
    if (!themePath.isEmpty() && QDir(themePath).exists())
        themePixmaps = loadPixmaps(themePath, themeName);

    // 3️⃣ Theme-Mapping vorbereiten
    QMap<QString, QMap<ControlState, QPixmap>> themeMap;

    auto processPixmaps = [&](const QMap<QString, QPixmap>& pixmaps) {
        for (auto it = pixmaps.begin(); it != pixmaps.end(); ++it) {
            const QString key = it.key().toLower();
            const QPixmap& pix = it.value();

            if (pix.isNull())
                continue;

            QMap<ControlState, QPixmap> states;

            // 🔍 4-State SpriteStrip?
            if (pix.width() >= pix.height() * 4)
                states = splitTextureStates(pix);
            else
                states[ControlState::Normal] = pix;

            themeMap[key] = states;
        }
    };

    // 4️⃣ Default zuerst, dann Custom drüber
    processPixmaps(defaultPixmaps);
    processPixmaps(themePixmaps);

    // 5️⃣ In globale Map eintragen
    m_themes.insert(themeName.toLower(), themeMap);

    qDebug() << "[ThemeManager] Stored theme:" << themeName.toLower()
             << "with" << themeMap.size() << "entries";


    // 6️⃣ Falls noch kein aktives Theme → setzen
    if (m_currentTheme.isEmpty())
        m_currentTheme = themeName.toLower();

    qInfo().noquote() << "[ThemeManager] Theme '" << themeName
                      << "' geladen (" << themeMap.size() << " Texturen)";

    emit texturesUpdated();
    return true;
}


bool ThemeManager::setCurrentTheme(const QString& themeName)
{
    QString lower = themeName.toLower();
    if (!m_themes.contains(lower)) {
        qWarning() << "[ThemeManager] Theme nicht gefunden:" << lower;
        return false;
    }

    if (m_currentTheme == lower)
        return true;

    m_currentTheme = lower;
    qInfo().noquote() << "[ThemeManager] Aktives Theme geändert zu:" << m_currentTheme;
    emit themeChanged(m_currentTheme);
    emit texturesUpdated();
    return true;
}

QMap<ControlState, QPixmap> ThemeManager::splitTextureStates(const QPixmap& src) const
{
    QMap<ControlState, QPixmap> map;
    if (src.isNull()) return map;

    int stateCount = 4; // normal / hover / pressed / disabled
    int w = src.width() / stateCount;
    int h = src.height();

    map[ControlState::Normal]   = src.copy(0 * w, 0, w, h);
    map[ControlState::Hover]    = src.copy(1 * w, 0, w, h);
    map[ControlState::Pressed]  = src.copy(2 * w, 0, w, h);
    map[ControlState::Disabled] = src.copy(3 * w, 0, w, h);

    return map;
}

QPixmap ThemeManager::texture(const QString& name, ControlState state) const
{
    if (m_currentTheme.isEmpty() || !m_themes.contains(m_currentTheme)) {
        qWarning() << "[ThemeManager] Kein aktives Theme!";
        return QPixmap();
    }

    QFileInfo fi(name);
    QString key = fi.completeBaseName().toLower();

    const auto& themeMap = m_themes.value(m_currentTheme);

    auto fetchPixmap = [&](const QMap<QString, QMap<ControlState, QPixmap>>& map,
                           const QString& debugName)
    {
        if (!map.contains(key))
            return QPixmap();

        const auto& states = map.value(key);

        if (states.contains(state))
            return states.value(state);

        if (states.contains(ControlState::Normal))
            return states.value(ControlState::Normal);

        return QPixmap();
    };

    // 1) Aktives Theme
    QPixmap pm = fetchPixmap(themeMap, "Active");
    if (!pm.isNull())
        return pm;

    // 2) Fallback: Default Theme
    if (m_themes.contains("default")) {
        const auto& defMap = m_themes.value("default");
        pm = fetchPixmap(defMap, "Default");

        if (!pm.isNull())
            return pm;
    }

    static QSet<QString> warned;
    if (!warned.contains(key)) {
        warned.insert(key);
        qWarning() << "[ThemeManager] Textur nicht gefunden:" << name;
    }

    return QPixmap();
}

bool ThemeManager::matchesFullTexture(const QPixmap& pm, int wndW, int wndH) const
{
    if (pm.isNull())
        return false;

    return pm.width() == wndW && pm.height() == wndH;
}

bool ThemeManager::hasTileSet(const QString& baseName) const
{
    for (int i = 0; i < 12; i++)
    {
        QString key = QString("%1%2").arg(baseName).arg(i, 2, 10, QChar('0'));

        if (textureFor(key).isNull())
            return false;
    }
    return true;
}

ThemeManager::WindowSkin ThemeManager::buildTileSet(const QString& baseName) const
{
    WindowSkin skin;

    for (int i = 0; i < 12; i++)
    {
        QString key = QString("%1%2").arg(baseName).arg(i, 2, 10, QChar('0'));

        skin.tiles[i] = textureFor(key);

        if (skin.tiles[i].isNull())
            return skin;
    }

    skin.isTileset = true;
    skin.valid = true;
    return skin;
}

QPixmap ThemeManager::textureFor(const QString& name, ControlState state) const
{
    if (!m_themes.contains(m_currentTheme))
        return QPixmap();

    const auto& theme = m_themes[m_currentTheme];

    if (!theme.contains(name))
        return QPixmap();

    const auto& states = theme[name];

    if (states.contains(state))
        return states[state];

    // Fallback auf Normal
    if (states.contains(ControlState::Normal))
        return states[ControlState::Normal];

    return QPixmap();
}

ThemeManager::WindowSkin ThemeManager::resolveWindowSkin(
    const QString& texName, int wndW, int wndH) const
{
    WindowSkin ws;

    if (texName.isEmpty())
        return ws;

    // 1) Prüfe TileSet
    if (hasTileSet(texName))
    {
        ws = buildTileSet(texName);
        if (ws.valid)
            return ws;
    }

    // 2) Prüfe FullTexture
    QPixmap pm = textureFor(texName);
    if (matchesFullTexture(pm, wndW, wndH))
    {
        ws.tiles[0] = pm;
        ws.valid = true;
        ws.isTileset = false;
        return ws;
    }

    // 3) Fallback (keine Erkennung)
    return ws;
}

