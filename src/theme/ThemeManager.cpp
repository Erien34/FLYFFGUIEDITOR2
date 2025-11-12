#include "ThemeManager.h"
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
    QMap<QString, QPixmap> defaultPixmaps = ResourceUtils::loadPixmaps(defaultPath);

    // 2️⃣ Optionales Theme (z. B. English) laden
    QMap<QString, QPixmap> themePixmaps;
    if (!themePath.isEmpty() && QDir(themePath).exists())
        themePixmaps = ResourceUtils::loadPixmaps(themePath);

    // 3️⃣ Theme-Mapping vorbereiten
    QMap<QString, QMap<ControlState, QPixmap>> themeMap;

    auto processPixmaps = [&](const QMap<QString, QPixmap>& pixmaps) {
        for (auto it = pixmaps.begin(); it != pixmaps.end(); ++it) {
            const QString key = it.key().toLower();
            const QPixmap& pix = it.value();

            if (pix.isNull())
                continue;

            QMap<ControlState, QPixmap> states;

            // 🔍 Wenn das Bild viermal breiter als hoch ist → SpriteStrip mit 4 States
            if (pix.width() >= pix.height() * 4)
                states = splitTextureStates(pix);
            else
                states[ControlState::Normal] = pix;

            themeMap.insert(key, states);
        }
    };

    // 4️⃣ Default zuerst, dann Custom drüber
    processPixmaps(defaultPixmaps);
    processPixmaps(themePixmaps);

    // 5️⃣ In globale Map eintragen
    m_themes.insert(themeName.toLower(), themeMap);

    // 6️⃣ Falls noch kein aktives Theme → dieses setzen
    if (m_currentTheme.isEmpty())
        m_currentTheme = themeName.toLower();

    qInfo().noquote() << QString("[ThemeManager] Theme '%1' geladen (%2 Texturen)")
                             .arg(themeName)
                             .arg(themeMap.size());

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

const QPixmap& ThemeManager::texture(const QString& key, ControlState state) const
{
    static QPixmap dummy;

    // Kein aktives Theme → Dummy
    if (m_currentTheme.isEmpty() || !m_themes.contains(m_currentTheme)) {
        qWarning() << "[ThemeManager] Kein aktives Theme gesetzt oder gefunden!";
        return dummy;
    }

    const QString lowerKey = key.toLower();
    const auto& currentThemeMap = m_themes.value(m_currentTheme);

    // 1️⃣ Suche im aktiven Theme
    if (currentThemeMap.contains(lowerKey)) {
        const auto& states = currentThemeMap.value(lowerKey);
        if (states.contains(state))
            return states.value(state);
        if (states.contains(ControlState::Normal))
            return states.value(ControlState::Normal);
    }

    // 2️⃣ Fallback: Default-Theme
    if (m_themes.contains("default")) {
        const auto& defaultThemeMap = m_themes.value("default");
        if (defaultThemeMap.contains(lowerKey)) {
            const auto& states = defaultThemeMap.value(lowerKey);
            if (states.contains(state))
                return states.value(state);
            if (states.contains(ControlState::Normal))
                return states.value(ControlState::Normal);
        }
    }

    qWarning() << "[ThemeManager] Textur nicht gefunden:" << key;
    return dummy;
}
