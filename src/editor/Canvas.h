#pragma once
#include "WindowData.h"
#include <QWidget>

class ProjectController;
class RenderManager;
class CanvasHandler;


class Canvas : public QWidget {
    Q_OBJECT
public:
    explicit Canvas(ProjectController* controller, QWidget* parent = nullptr);
    ~Canvas();

    void setActiveWindow(const std::shared_ptr<WindowData>& wnd);

protected:
    void paintEvent(QPaintEvent* event) override;
    // void mousePressEvent(QMouseEvent* event) override;
    // void mouseMoveEvent(QMouseEvent* event) override;
    // void mouseReleaseEvent(QMouseEvent* event) override;
    // void keyPressEvent(QKeyEvent* event) override;
    // void keyReleaseEvent(QKeyEvent* event) override;

private:
    ProjectController* m_controller =nullptr;
    RenderManager* m_renderManager =nullptr;
    CanvasHandler* m_handler =nullptr;

    std::shared_ptr<WindowData> m_activeWindow;
};
