#pragma once
#include <QObject>
#include <QPainter>
#include <memory>

#include "ThemeManager.h"
#include "BehaviorManager.h"
#include "WindowData.h"

class RenderWindow : public QObject
{
    Q_OBJECT

public:
    explicit RenderWindow(ThemeManager* themeManager,
                          BehaviorManager* behaviorManager,
                          QObject* parent = nullptr);

    // Neues, einzig gültiges Render
    void render(QPainter& p,
                const std::shared_ptr<WindowData>& wnd,
                const QSize& canvasSize);

private:
    // Render-Teile
    bool drawDirectWindowTexture(QPainter& p,
                                 const std::shared_ptr<WindowData>& wnd,
                                 const QRect& wndRect);

    bool drawWindowTileset(QPainter& p, const QRect& area);

    void drawFallbackWindow(QPainter& p, const QRect& wndRect);

    void drawTitleAndButtons(QPainter& p,
                             const std::shared_ptr<WindowData>& wnd,
                             const QRect& wndRect);

    // Single buttons
    void drawCloseButton(QPainter* painter, const QRect& rect);
    void drawHelpButton(QPainter* painter, const QRect& rect);
    void drawFallbackButton(QPainter& p, const QRect& rect, const QColor& color, const QString& text);

private:
    ThemeManager* m_themeManager = nullptr;
    BehaviorManager* m_behaviorManager = nullptr;
};
