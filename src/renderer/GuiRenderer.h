#pragma once
#include <QObject>
#include <QPainter>
#include <memory>
#include <vector>

#include "layout/model/WindowData.h"
#include "layout/model/ControlData.h"
#include "defines/BehaviorManager.h"
#include "RenderWindow.h"
#include "RenderControls.h"

// --------------------------------------------------
// ControlState – interner Renderzustand
// --------------------------------------------------
enum class ControlState {
    Normal,
    Hover,
    Pressed,
    Disabled
};

// ==================================================
// GuiRenderer – zentrale Renderklasse
//  Dirigiert Fenster- und Control-Renderer,
//  bezieht Behavior-Infos und Themes
// ==================================================
class GuiRenderer : public QObject
{
    Q_OBJECT
public:
    explicit GuiRenderer(BehaviorManager* behaviorMgr);

    // Haupt-Render-Funktion
    void render(QPainter& painter,
                const std::vector<std::shared_ptr<WindowData>>& windows);

    // Theme / Farben (optional)
    void setThemeColor(const QColor& accent);
    QColor themeColor() const { return m_themeColor; }

private:
    BehaviorManager* m_behaviorMgr;
    RenderWindow     m_windowRenderer;
    RenderControls   m_controlRenderer;

    QColor m_themeColor = QColor(255, 255, 255);

    void renderWindow(QPainter& p, const std::shared_ptr<WindowData>& wnd);
    void renderControl(QPainter& p, const std::shared_ptr<ControlData>& ctrl);
    ControlState stateFor(const std::shared_ptr<ControlData>& ctrl) const;
};
