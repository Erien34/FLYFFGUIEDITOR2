#pragma once
#include <QObject>
#include <QMap>
#include <QPixmap>
#include "ControlState.h"
#include "FileManager.h"
#include "TokenData.h"

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(FileManager* fileMgr, QObject* parent = nullptr);

struct WindowSkin {
        QPixmap tiles[12];      // 00 .. 11
        bool isTileset = false; // true = tile-based window
        bool valid = false;
};
    void refreshFromTokens(const QList<Token>& tokens);

    bool loadTheme(const QString& themeName);         // lädt (und cached) ein Theme
    bool setCurrentTheme(const QString& themeName);   // wechselt aktiv verwendetes Theme
    QString currentTheme() const { return m_currentTheme; }

    QPixmap texture(const QString& key, ControlState state) const;

    WindowSkin resolveWindowSkin(const QString& texName, int wndW, int wndH) const;

signals:
    void texturesUpdated();
    void themeChanged(const QString& name);

private:
    void clear();
    QMap<ControlState, QPixmap> splitTextureStates(const QPixmap& src) const;

    bool hasTileSet(const QString& baseName) const;
    WindowSkin buildTileSet(const QString& baseName) const;
    bool matchesFullTexture(const QPixmap& pm, int wndW, int wndH) const;

    QPixmap textureFor(const QString& name, ControlState state = ControlState::Normal) const;

    QMap<QString, QPixmap> loadPixmaps(const QString& path,
                                       const QString& themeName);

    FileManager* m_fileMgr = nullptr;
    QString m_currentTheme;

    // 🌍 Alle geladenen Themes: m_themes["default"]["buttwndexit"][Normal]
    QMap<QString, QMap<QString, QMap<ControlState, QPixmap>>> m_themes;
};
