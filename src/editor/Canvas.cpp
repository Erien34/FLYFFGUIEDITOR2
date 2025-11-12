#include "Canvas.h"
#include "RenderManager.h"
#include "CanvasHandler.h"
#include "ProjectController.h"
#include "WindowData.h"
#include <QPainter>
#include <QDebug>

Canvas::Canvas(ProjectController* controller, QWidget* parent)
    : QWidget(parent),
    m_controller(controller),
    m_renderManager(controller ? controller->renderManager() : nullptr),
    m_handler(new CanvasHandler(this, controller))
{
    setMinimumSize(800, 600);
    setMouseTracking(true);

    if (m_renderManager) {
        connect(m_renderManager, &RenderManager::requestRepaint,
                this, QOverload<>::of(&Canvas::update));
    }

    qInfo() << "[Canvas] Initialized with RenderManager:"
            << (m_renderManager ? "OK" : "null");
}

Canvas::~Canvas()
{
    delete m_handler;
}

void Canvas::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(45, 45, 45));

    if (m_renderManager && m_activeWindow)
        m_renderManager->render(&painter, m_activeWindow);
    else
        painter.drawText(rect(), Qt::AlignCenter, "RenderManager nicht verfügbar");
}

void Canvas::setActiveWindow(const std::shared_ptr<WindowData>& wnd)
{
    m_activeWindow = wnd;
    update();
}

// void Canvas::mousePressEvent(QMouseEvent* event) { m_handler->onMousePress(event); }
// void Canvas::mouseMoveEvent(QMouseEvent* event) { m_handler->onMouseMove(event); }
// void Canvas::mouseReleaseEvent(QMouseEvent* event) { m_handler->onMouseRelease(event); }
// void Canvas::keyPressEvent(QKeyEvent* event) { m_handler->onKeyPress(event); }
// void Canvas::keyReleaseEvent(QKeyEvent* event) { m_handler->onKeyRelease(event); }
