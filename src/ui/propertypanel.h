#pragma once
#include "layout/model/WindowData.h"
#include "layout/model/ControlData.h"
#include <QWidget>
#include <QTreeWidget>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QJsonObject>
#include <memory>

struct WindowData;
struct ControlData;
class ProjectController;

class PropertyPanel : public QWidget
{
    Q_OBJECT
public:
    explicit PropertyPanel(ProjectController* controller, QWidget* parent = nullptr);

    void showWindowProps(const std::shared_ptr<WindowData>& wnd);
    void showControlProps(const std::shared_ptr<WindowData>& wnd,
                          const std::shared_ptr<ControlData>& ctrl);
    void clear();

public slots:
    void refreshAfterLayoutLoad();

signals:
    void flagsChanged(quint32 newMask);

private:
    QWidget* createFlagGroup(const QString& title,
                             const QMap<QString, quint32>& allFlags,
                             const QStringList& activeFlags,
                             const QJsonObject& rules,
                             bool isWindow);

private:
    ProjectController* m_controller = nullptr;

    QVBoxLayout* m_layout = nullptr;
    QTreeWidget* m_tree = nullptr;

    QWidget* m_windowFlagGroup = nullptr;
    QWidget* m_controlFlagGroup = nullptr;
    WindowData* m_currentWindow = nullptr;
    ControlData* m_currentControl = nullptr;

    bool m_isRefreshing = false;
};
