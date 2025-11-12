#pragma once
#include <QObject>
#include <QMap>
#include <QPixmap>

class ThemeManager : public QObject {
    Q_OBJECT
public:
    explicit ThemeManager(QObject* parent = nullptr);

    bool load(const QString& path);
    bool setActiveTheme(const QString& name);

signals:
    void themeChanged(const QString& name);

private:
    QMap<QString, QPixmap> m_pixmaps;
    QString m_activeTheme;
};
