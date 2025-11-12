#include "render/RenderManager.h"
#include "render/RenderWindow.h"
#include "render/RenderControls.h"
#include "theme/ThemeManager.h"
#include "behavior/BehaviorManager.h"
#include <QDebug>

RenderManager::RenderManager(ThemeManager* themeMgr,
                             BehaviorManager* behaviorMgr,
                             QObject* parent)
    : QObject(parent),
    m_themeMgr(themeMgr),
    m_behaviorMgr(behaviorMgr)
{
    m_windowRenderer  = std::make_unique<RenderWindow>(this);
    m_controlRenderer = std::make_unique<RenderControls>(this);

    qInfo() << "[RenderManager] Initialisiert mit:"
            << "ThemeManager:" << (themeMgr ? "OK" : "null")
            << "BehaviorManager:" << (behaviorMgr ? "OK" : "null");
}

void RenderManager::refresh()
{
    emit requestRepaint();
}

void RenderManager::render(QPainter& painter, const QSize& size)
{
    painter.save();

    // Hintergrund und Rahmen
    painter.fillRect(QRect(QPoint(0, 0), size), QColor(45, 45, 45));
    painter.setPen(Qt::gray);
    painter.drawRect(QRect(QPoint(0, 0), size - QSize(1, 1)));
    painter.drawText(10, 20, "[RenderManager] aktiv");

    painter.restore();

    // Fenster zeichnen
    m_windowRenderer->render(painter);

    // Controls zeichnen
    m_controlRenderer->render(painter);
}
