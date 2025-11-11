#pragma once

#include <QObject>
#include <QPainter>
#include <QMap>
#include <QPixmap>
#include <QColor>
#include <memory>
#include <vector>

#include "layout/model/WindowData.h"
#include "layout/model/ControlData.h"
#include "renderer/ControlState.h"

// Forward-Decl – tatsächliche Definition steht in defines/BehaviorManager.h
class BehaviorManager;

// ==================================================
// GuiRenderer – zentrale Renderklasse
//  Dirigiert Fenster- und Control-Rendering,
//  später inkl. Behavior & States
// ==================================================
class GuiRenderer : public QObject
{
    Q_OBJECT
public:
    explicit GuiRenderer(BehaviorManager* behaviorMgr = nullptr,
                         QObject* parent = nullptr);

    // Haupt-Render-Funktion
    void render(QPainter& painter,
                const std::vector<std::shared_ptr<WindowData>>& windows,
                const QMap<QString, QPixmap>& themes,
                const QSize& canvasSize);

    // Theme / Farben (optional)
    void setThemeColor(const QColor& accent);
    QColor themeColor() const { return m_themeColor; }

private:
    BehaviorManager* m_behaviorMgr = nullptr;
    QColor m_themeColor = QColor(255, 255, 255);

    void renderWindow(QPainter& p,
                      const std::shared_ptr<WindowData>& wnd,
                      const QMap<QString, QPixmap>& themes,
                      const QSize& canvasSize);

    void renderControl(QPainter& p,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QRect& rect,
                       const QMap<QString, QPixmap>& themes);

    ControlState stateFor(const std::shared_ptr<ControlData>& ctrl) const;
};
