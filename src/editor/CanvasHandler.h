#pragma once
#include <QObject>
#include <memory>

class ProjectController;
class Canvas;
struct WindowData;

class CanvasHandler : public QObject
{
    Q_OBJECT
public:
    explicit CanvasHandler(ProjectController* controller, QObject* parent = nullptr);

    Canvas* canvas() const;

public slots:
    void showWindow(const std::shared_ptr<WindowData>& wnd);
    void onActiveWindowChanged(const std::shared_ptr<WindowData>& wnd);

private:
    ProjectController* m_controller = nullptr;
    Canvas* m_canvas = nullptr;
};
