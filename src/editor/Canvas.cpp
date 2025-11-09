#include "Canvas.h"
#include "core/ProjectController.h"
#include "layout/LayoutManager.h"
#include "layout/model/ControlData.h"

#include "renderer/RenderWindow.h"
#include "renderer/RenderControls.h"

#include <QPainter>
#include <QPaintEvent>
#include <QDebug>

Canvas::Canvas(ProjectController* controller, LayoutManager* lm, QWidget* parent)
    : QWidget(parent),
    m_controller(controller),
    m_layoutManager(lm)
{
    setMinimumSize(600, 400);
    setAutoFillBackground(false);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    setMouseTracking(true);
}

void Canvas::setActiveWindow(const std::shared_ptr<WindowData>& wnd)
{
    m_activeWindow = wnd;
    update();
}

void Canvas::setRenderMode(RenderMode mode)
{
    if (m_renderMode == mode)
        return;
    m_renderMode = mode;
    update();
}

// ======================================================
// Hintergrund (Gitter, neutraler Hintergrund)
// ======================================================
void Canvas::drawBackground(QPainter& p)
{
    p.fillRect(rect(), QColor(35, 35, 40));

    const int gridSize = 16;
    p.setPen(QColor(50, 50, 55));

    for (int x = 0; x < width(); x += gridSize)
        p.drawLine(x, 0, x, height());
    for (int y = 0; y < height(); y += gridSize)
        p.drawLine(0, y, width(), y);
}

// ======================================================
// Haupt-Rendering
// ======================================================
void Canvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);

    drawBackground(p);

    if (!m_layoutManager || !m_controller) {
        p.setPen(Qt::white);
        p.drawText(rect(), Qt::AlignCenter, "Fehlender Controller oder LayoutManager");
        return;
    }

    const auto& themes = m_controller->themes();
    if (themes.isEmpty()) {
        p.setPen(Qt::darkGray);
        p.drawText(rect(), Qt::AlignCenter, "Kein Theme geladen");
        return;
    }

    // Nur aktives Fenster
    if (m_renderMode == RenderMode::ActiveOnly) {
        if (!m_activeWindow) {
            p.setPen(Qt::white);
            p.drawText(rect(), Qt::AlignCenter, "Kein Fenster ausgewählt");
            return;
        }
        RenderWindow::renderWindow(p, m_activeWindow, themes, size());
        return;
    }

    // Alle Fenster rendern
    if (m_renderMode == RenderMode::AllWindows) {
        const auto& windows = m_layoutManager->processedWindows();
        for (const auto& wnd : windows) {
            if (wnd)
                RenderWindow::renderWindow(p, wnd, themes, size());
        }

        p.setPen(QColor(200, 200, 200));
        p.drawText(10, 20, "[Preview Mode – Alle Fenster]");
    }
}
