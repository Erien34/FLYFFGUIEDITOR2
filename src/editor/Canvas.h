#pragma once
#include <QWidget>
#include <memory>

#include "layout/model/WindowData.h"

class ProjectController;
class LayoutManager;

enum class RenderMode {
    ActiveOnly,
    AllWindows
};

class Canvas : public QWidget
{
    Q_OBJECT

public:
    explicit Canvas(ProjectController* controller, LayoutManager* lm, QWidget* parent = nullptr);

    void setActiveWindow(const std::shared_ptr<WindowData>& wnd);
    void setRenderMode(RenderMode mode);
    RenderMode renderMode() const { return m_renderMode; }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    ProjectController*              m_controller = nullptr;
    LayoutManager*                  m_layoutManager = nullptr;
    std::shared_ptr<WindowData>     m_activeWindow;
    RenderMode                      m_renderMode = RenderMode::ActiveOnly;

    void drawBackground(QPainter& p);
};
