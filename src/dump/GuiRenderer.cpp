#include "renderer/GuiRenderer.h"

#include "renderer/RenderWindow.h"
#include "renderer/RenderControls.h"
#include <QDebug>

GuiRenderer::GuiRenderer(BehaviorManager* behaviorMgr, QObject* parent)
    : QObject(parent),
    m_behaviorMgr(behaviorMgr)
{
    if (!m_behaviorMgr) {
        qInfo() << "[GuiRenderer] BehaviorManager ist null – Behaviors aktuell inaktiv.";
    }
}

void GuiRenderer::setThemeColor(const QColor& accent)
{
    m_themeColor = accent;
}

// ------------------------------------------------------------
// Gesamtes Layout rendern (alle Fenster & Controls)
// ------------------------------------------------------------
void GuiRenderer::render(QPainter& painter,
                         const std::vector<std::shared_ptr<WindowData>>& allWindows,
                         const QMap<QString, QPixmap>& themes,
                         const QSize& canvasSize)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // 🧭 1️⃣ Welche Fenster?
    std::vector<std::shared_ptr<WindowData>> targets;

    if (m_renderMode == RenderMode::ActiveOnly) {
        if (m_activeWindow)
            targets.push_back(m_activeWindow);
    } else {
        targets = allWindows;
    }

    // 🖼️ 2️⃣ Rendern
    for (const auto& wnd : targets) {
        if (!wnd || !wnd->valid) continue;

        RenderWindow::renderWindow(painter, wnd, themes, canvasSize);

        for (const auto& ctrl : wnd->controls) {
            if (!ctrl || !ctrl->valid) continue;

            QRect rect(ctrl->x1, ctrl->y1, ctrl->x2 - ctrl->x1, ctrl->y2 - ctrl->y1);
            RenderControls::renderControl(painter, rect, ctrl, themes, ControlState::Normal);
        }
    }

    // 🧩 3️⃣ Optional: Mode-Overlay (debug/preview)
    if (m_renderMode == RenderMode::AllWindows) {
        painter.setPen(QColor(200, 200, 200));
        painter.drawText(10, 20, "[Preview Mode – Alle Fenster]");
    }

    painter.restore();
}

// ------------------------------------------------------------
// Einzelnes Fenster (Rahmen + Controls)
// ------------------------------------------------------------
void GuiRenderer::renderWindow(QPainter& p,
                               const std::shared_ptr<WindowData>& wnd,
                               const QMap<QString, QPixmap>& themes,
                               const QSize& canvasSize)
{
    if (!wnd)
        return;

    // 1️⃣ Fensterrahmen + Titel zeichnen
    RenderWindow::renderWindow(p, wnd, themes, canvasSize);

    // 2️⃣ Fensterposition auf Canvas bestimmen (zentriert)
    QRect wndRect(0, 0, wnd->width, wnd->height);
    QPoint center(canvasSize.width()  / 2 - wndRect.width()  / 2,
                  canvasSize.height() / 2 - wndRect.height() / 2);
    wndRect.moveTopLeft(center);

    // 3️⃣ ClientRect (Innenraum) vom Fenster holen
    QRect client = RenderWindow::getClientRect(wnd, themes);

    // 4️⃣ Controls des Fensters zeichnen
    for (const auto& ctrl : wnd->controls)
    {
        if (!ctrl || !ctrl->valid)
            continue;

        QRect rect(ctrl->x1 + wndRect.x() + client.left(),
                   ctrl->y1 + wndRect.y() + client.top(),
                   ctrl->x2 - ctrl->x1,
                   ctrl->y2 - ctrl->y1);

        // Clipping auf Fensterbereich
        rect = rect.intersected(wndRect.adjusted(0, 0, -1, -1));
        if (rect.isEmpty())
            continue;

        renderControl(p, ctrl, rect, themes);
    }
}

// ------------------------------------------------------------
// Einzelnes Control zeichnen (Platz für Behavior/State)
// ------------------------------------------------------------
void GuiRenderer::renderControl(QPainter& p,
                                const std::shared_ptr<ControlData>& ctrl,
                                const QRect& rect,
                                const QMap<QString, QPixmap>& themes)
{
    if (!ctrl)
        return;

    // 🔹 später: Behaviors auswerten
    Q_UNUSED(m_behaviorMgr);

    ControlState state = stateFor(ctrl);
    Q_UNUSED(state); // aktuell noch nicht genutzt

    // DEBUG: jede Control-Box einfärben
    p.save();
    p.setPen(Qt::yellow);
    p.drawRect(rect.adjusted(0, 0, -1, -1));
    p.restore();

    // Aktuell: weiterleiten an den typbasierten Renderer
    RenderControls::renderControl(p, rect, ctrl, themes, state);
}

// ------------------------------------------------------------
// Dummy-State-Ermittlung (wird später erweitert)
// ------------------------------------------------------------
ControlState GuiRenderer::stateFor(const std::shared_ptr<ControlData>& ctrl) const
{
    if (!ctrl)
        return ControlState::Normal;

    if (ctrl->disabled)
        return ControlState::Disabled;

    if (ctrl->isPressed)
        return ControlState::Pressed;

    if (ctrl->isHovered)
        return ControlState::Hover;

    return ControlState::Normal;
}
