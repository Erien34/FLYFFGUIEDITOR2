#include "GuiRenderer.h"
#include <QDebug>

GuiRenderer::GuiRenderer(BehaviorManager* behaviorMgr)
    : m_behaviorMgr(behaviorMgr)
{
    Q_ASSERT(m_behaviorMgr);
}

void GuiRenderer::setThemeColor(const QColor& accent)
{
    m_themeColor = accent;
}

// ------------------------------------------------------------
// Gesamtes Layout rendern (alle Fenster & Controls)
// ------------------------------------------------------------
void GuiRenderer::render(QPainter& painter,
                         const std::vector<std::shared_ptr<WindowData>>& windows)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    for (const auto& wnd : windows)
    {
        if (!wnd || !wnd->valid)
            continue; // überspringe ungültige Fenster

        renderWindow(painter, wnd);
    }

    painter.restore();
}

// ------------------------------------------------------------
// Einzelnes Fenster zeichnen (inkl. Controls)
// ------------------------------------------------------------
void GuiRenderer::renderWindow(QPainter& p,
                               const std::shared_ptr<WindowData>& wnd)
{
    // Fenster selbst rendern
    m_windowRenderer.renderWindow(p, *wnd);

    // Alle Controls dieses Fensters zeichnen
    for (const auto& ctrl : wnd->controls)
    {
        if (!ctrl || !ctrl->valid)
            continue;

        renderControl(p, ctrl);
    }
}

// ------------------------------------------------------------
// Einzelnes Control zeichnen (inkl. State + Theme)
// ------------------------------------------------------------
void GuiRenderer::renderControl(QPainter& p,
                                const std::shared_ptr<ControlData>& ctrl)
{
    BehaviorInfo info = m_behaviorMgr->resolveBehavior(*ctrl);
    ControlState state = stateFor(ctrl);

    QColor drawColor = ctrl->color.isValid()
                           ? ctrl->color
                           : QColor(255, 255, 255);

    // einfache Farbvariation nach State
    switch (state) {
    case ControlState::Hover:
        drawColor = drawColor.lighter(115);
        break;
    case ControlState::Pressed:
        drawColor = drawColor.darker(120);
        break;
    case ControlState::Disabled:
        drawColor.setAlpha(100);
        break;
    default:
        break;
    }

    p.save();
    m_controlRenderer.renderControl(p, *ctrl, info, drawColor);
    p.restore();
}

// ------------------------------------------------------------
// Dummy-State-Ermittlung (später via CanvasHandler ersetzen)
// ------------------------------------------------------------
ControlState GuiRenderer::stateFor(const std::shared_ptr<ControlData>& ctrl) const
{
    Q_UNUSED(ctrl);
    // Aktuell haben wir keine State-Flags – einfach Normal
    return ControlState::Normal;
}
