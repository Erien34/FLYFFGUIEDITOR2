#pragma once
#include "WindowData.h"
#include "RenderWindow.h"
#include <QObject>
#include <QPainter>

class ThemeManager;
class BehaviorManager;
class RenderWindow;
//class RenderControls;

class RenderManager : public QObject {
    Q_OBJECT
public:
    RenderManager(ThemeManager* themeMgr, BehaviorManager* behaviorMgr);

    // Zeichnet den gesamten Canvas
    void render(QPainter* painter, const std::shared_ptr<WindowData>& wnd);


signals:
    // Wird gesendet, wenn eine Neuzeichnung nötig ist
    void requestRepaint();

public slots:
    // Erzwingt eine Neuzeichnung
    void refresh();

private:

    ThemeManager* m_themeManager;
    BehaviorManager* m_behaviorManager;
    std::unique_ptr<RenderWindow> m_windowRenderer;

    //std::unique_ptr<RenderControl> m_controlRenderer;
};
