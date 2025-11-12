#pragma once
#include <QObject>
#include <QPainter>
#include <QPixmap>
#include "layout/model/WindowData.h"
#include "theme/ThemeManager.h"
#include "behavior/BehaviorManager.h"

class RenderWindow : public QObject
{
    Q_OBJECT
public:
    explicit RenderWindow(ThemeManager* themeManager,
                          BehaviorManager* behaviorManager,
                          QObject* parent = nullptr);

    void render(QPainter* painter, const WindowData& window);

private:
    ThemeManager* m_themeManager;
    BehaviorManager* m_behaviorManager;

    void drawFrame(QPainter* painter, const QRect& rect);
    void drawCloseButton(QPainter* painter, const QRect& rect);
    void drawHelpButton(QPainter* painter, const QRect& rect);
};
