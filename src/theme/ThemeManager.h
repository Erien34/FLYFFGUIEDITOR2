#pragma once
#include <QObject>
#include <QMap>
#include <QPixmap>
#include "ControlState.h"
#include "FileManager.h"
#include "TokenData.h"
#include "ResourceUtils.h"

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(FileManager* fileMgr, QObject* parent = nullptr);

    void refreshFromTokens(const QList<Token>& tokens);

    bool loadTheme(const QString& themeName);         // lädt (und cached) ein Theme
    bool setCurrentTheme(const QString& themeName);   // wechselt aktiv verwendetes Theme
    QString currentTheme() const { return m_currentTheme; }

    const QPixmap& texture(const QString& key, ControlState state = ControlState::Normal) const;

signals:
    void texturesUpdated();
    void themeChanged(const QString& name);

private:
    void clear();
    QMap<ControlState, QPixmap> splitTextureStates(const QPixmap& src) const;

    FileManager* m_fileMgr = nullptr;
    QString m_currentTheme;

    // 🌍 Alle geladenen Themes: m_themes["default"]["buttwndexit"][Normal]
    QMap<QString, QMap<QString, QMap<ControlState, QPixmap>>> m_themes;
};
