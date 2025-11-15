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

void RenderManager::render(QPainter* painter,
                           const std::shared_ptr<WindowData>& wnd)
{
    if (!painter || !wnd || !m_windowRenderer)
        return;

    // 1) Hintergrund zeichnen (Canvas)
    painter->save();

    const int canvasW = painter->device()->width();
    const int canvasH = painter->device()->height();
    QRect canvasRect(0, 0, canvasW, canvasH);

    painter->fillRect(canvasRect, QColor(45, 45, 45));
    painter->setPen(Qt::gray);
    painter->drawRect(canvasRect.adjusted(0, 0, -1, -1));
    painter->drawText(10, 20, "[RenderManager] aktiv");

    painter->restore();

    // 2) Canvasgröße an RenderWindow übergeben
    QSize canvasSize(canvasW, canvasH);

    // 3) Fenster vollständig durch RenderWindow rendern
    //    (korrekte Signatur mit Referenz!)
    m_windowRenderer->render(*painter, wnd, canvasSize);
}
