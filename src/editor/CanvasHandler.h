#pragma once
#include "WindowData.h"
#include <QObject>

class ProjectController;
class QMouseEvent;
class QKeyEvent;
class Canvas;

class CanvasHandler : public QObject {
    Q_OBJECT
public:
    CanvasHandler(Canvas* canvas, ProjectController* controller);

    Canvas* canvas() const { return m_canvas; }

    // void onMousePress(QMouseEvent* event);
    // void onMouseMove(QMouseEvent* event);
    // void onMouseRelease(QMouseEvent* event);
    // void onKeyPress(QKeyEvent* event);
    // void onKeyRelease(QKeyEvent* event);

public slots:
    void showWindow(const std::shared_ptr<WindowData>& wnd);
    void onActiveWindowChanged(const std::shared_ptr<WindowData>& wnd);
private:
    Canvas* m_canvas;
    ProjectController* m_controller;
};
