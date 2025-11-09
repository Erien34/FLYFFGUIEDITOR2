#pragma once
#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QVBoxLayout>
#include <memory>
#include "layout/model/WindowData.h"
#include "layout/model/ControlData.h"

class ProjectController;

/**
 * WindowPanel
 * -----------------------------------------------------
 * Zeigt alle Fenster und deren Controls im Projekt an.
 * Arbeitet direkt mit dem ProjectController.
 */
class WindowPanel : public QWidget
{
    Q_OBJECT

public:
    explicit WindowPanel(ProjectController* controller, QWidget* parent = nullptr);

    // Liste neu aufbauen, wenn Layout neu geladen wurde
    void updateWindowList();
    void forceRefreshIfEmpty();

signals:
    void windowSelected(const QString& windowName);
    void controlSelected(const QString& windowName, const QString& controlId);

private slots:
    void onSearchTextChanged(const QString& text);
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    ProjectController* m_controller = nullptr;
    QLineEdit* m_searchBox = nullptr;
    QTreeWidget* m_tree = nullptr;

    bool m_isRefreshing = false;
    std::vector<std::shared_ptr<WindowData>> m_windows;
};
