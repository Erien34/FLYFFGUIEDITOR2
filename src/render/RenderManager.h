#pragma once
#include <QObject>
#include <QPainter>

class ThemeManager;
class BehaviorManager;
class RenderWindow;
class RenderControls;

class RenderManager : public QObject {
    Q_OBJECT
public:
    explicit RenderManager(ThemeManager* themeMgr,
                           BehaviorManager* behaviorMgr,
                           QObject* parent = nullptr);

    // Zeichnet den gesamten Canvas
    void render(QPainter& painter, const QSize& size);

signals:
    // Wird gesendet, wenn eine Neuzeichnung nötig ist
    void requestRepaint();

public slots:
    // Erzwingt eine Neuzeichnung
    void refresh();

private:
    ThemeManager*    m_themeMgr        = nullptr;
    BehaviorManager* m_behaviorMgr     = nullptr;

    std::unique_ptr<RenderWindow> m_windowRenderer;
    std::unique_ptr<RenderControls> m_controlRenderer;
};
