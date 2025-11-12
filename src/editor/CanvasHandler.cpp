#include "CanvasHandler.h"
#include "ProjectController.h"
#include "Canvas.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QDebug>

CanvasHandler::CanvasHandler(Canvas* canvas, ProjectController* controller)
    : QObject(nullptr),
    m_canvas(canvas),
    m_controller(controller)
{
}

void CanvasHandler::showWindow(const std::shared_ptr<WindowData>& wnd)
{
    if (!m_canvas || !wnd)
        return;

    qInfo().noquote() << QString("[CanvasHandler] Zeige Fenster '%1' an").arg(wnd->name);
    m_canvas->setActiveWindow(wnd);
    m_canvas->update();
}

void CanvasHandler::onActiveWindowChanged(const std::shared_ptr<WindowData>& wnd)
{
    showWindow(wnd);
}

// void CanvasHandler::onMousePress(QMouseEvent* event)  { qDebug() << "Mouse Press" << event->pos(); }
// void CanvasHandler::onMouseMove(QMouseEvent* event)   { qDebug() << "Mouse Move" << event->pos(); }
// void CanvasHandler::onMouseRelease(QMouseEvent* event){ qDebug() << "Mouse Release" << event->pos(); }
// void CanvasHandler::onKeyPress(QKeyEvent* event)      { qDebug() << "Key Press:" << event->key(); }
// void CanvasHandler::onKeyRelease(QKeyEvent* event)    { qDebug() << "Key Release:" << event->key(); }
