#include "CanvasHandler.h"
#include "Canvas.h"
#include "core/ProjectController.h"
#include "layout/LayoutManager.h"
#include "layout/model/WindowData.h"

#include <QDebug>

CanvasHandler::CanvasHandler(ProjectController* controller, QObject* parent)
    : QObject(parent), m_controller(controller)
{
    if (!m_controller) {
        qCritical() << "[CanvasHandler] Kein gültiger Controller!";
        return;
    }

    auto* lm = m_controller->layoutManager();
    if (!lm) {
        qCritical() << "[CanvasHandler] Kein LayoutManager im Controller gefunden!";
        return;
    }

    // 🔹 Canvas erzeugen mit Controller + LayoutManager
    m_canvas = new Canvas(m_controller, lm, nullptr);
    qInfo() << "[CanvasHandler] Canvas erfolgreich erstellt.";
}

Canvas* CanvasHandler::canvas() const
{
    return m_canvas;
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
