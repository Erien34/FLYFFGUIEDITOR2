#include "ThemeManager.h"
#include <QDir>
#include <QDebug>

ThemeManager::ThemeManager(QObject* parent)
    : QObject(parent)
{
}

bool ThemeManager::load(const QString& path)
{
    QDir dir(path);
    if (!dir.exists()) {
        qWarning() << "[ThemeManager] Pfad existiert nicht:" << path;
        return false;
    }

    for (const QString& f : dir.entryList(QStringList() << "*.png" << "*.jpg" << "*.bmp")) {
        m_pixmaps.insert(f, QPixmap(dir.filePath(f)));
    }

    qInfo() << "[ThemeManager] Texturen geladen:" << m_pixmaps.size();
    return true;
}

bool ThemeManager::setActiveTheme(const QString& name)
{
    if (name == m_activeTheme) return false;
    m_activeTheme = name;
    emit themeChanged(name);
    qInfo() << "[ThemeManager] Aktives Theme:" << name;
    return true;
}
