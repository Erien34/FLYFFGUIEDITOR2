#include "RenderManager.h"
#include "RenderWindow.h"
#include "theme/ThemeManager.h"
#include "behavior/BehaviorManager.h"
#include "layout/model/WindowData.h"
#include <QDebug>

RenderManager::RenderManager(ThemeManager* themeMgr, BehaviorManager* behaviorMgr)
    : m_themeManager(themeMgr),
    m_behaviorManager(behaviorMgr)
{
    m_windowRenderer = std::make_unique<RenderWindow>(themeMgr, behaviorMgr);
   // m_controlRenderer = std::make_unique<RenderControls>(this);

    qInfo() << "[RenderManager] Initialisiert mit:"
            << "ThemeManager:" << (themeMgr ? "OK" : "null")
            << "BehaviorManager:" << (behaviorMgr ? "OK" : "null");
}

void RenderManager::refresh()
{
    emit requestRepaint();
}

void RenderManager::render(QPainter* painter, const std::shared_ptr<WindowData>& wnd)
{
    if (!painter || !wnd)
        return;

    painter->save();

    // Hintergrund (Canvas)
    painter->fillRect(QRect(0, 0, 800, 600), QColor(45, 45, 45));
    painter->setPen(Qt::gray);
    painter->drawRect(0, 0, 799, 599);
    painter->drawText(10, 20, "[RenderManager] aktiv");

    painter->restore();

    // Fenster vollständig durch RenderWindow rendern
    m_windowRenderer->render(painter, *wnd);
}
