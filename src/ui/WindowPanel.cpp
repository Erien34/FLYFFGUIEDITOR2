#include "WindowPanel.h"
#include "core/ProjectController.h"
#include "layout/LayoutManager.h"
#include <QTreeWidgetItem>
#include <QDebug>

WindowPanel::WindowPanel(ProjectController* controller, QWidget* parent)
    : QWidget(parent),
    m_controller(controller)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(4);

    // 🔍 Suchfeld
    m_searchBox = new QLineEdit(this);
    m_searchBox->setPlaceholderText("Suche Fenster oder Control...");
    layout->addWidget(m_searchBox);

    // 🌲 Tree-Widget
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderHidden(true);
    layout->addWidget(m_tree, 1);

    setLayout(layout);

    // 🔗 Signale
    connect(m_tree, &QTreeWidget::itemClicked,
            this, &WindowPanel::onItemClicked);
    connect(m_searchBox, &QLineEdit::textChanged,
            this, &WindowPanel::onSearchTextChanged);
}

void WindowPanel::updateWindowList()
{
    if (!m_controller || !m_controller->layoutManager())
        return;

    auto lm = m_controller->layoutManager();
    m_windows = lm->processedWindows();

    qInfo() << "[WindowPanel] updateWindowList() -> empfange" << m_windows.size() << "Fenster.";

    m_tree->clear();

    if (m_windows.empty()) {
        auto* emptyItem = new QTreeWidgetItem({"Keine Fenster gefunden."});
        emptyItem->setFlags(Qt::NoItemFlags);
        m_tree->addTopLevelItem(emptyItem);
        return;
    }

    for (const auto& wnd : m_windows) {
        if (!wnd) continue;

        auto* wndItem = new QTreeWidgetItem({ wnd->name });
        wndItem->setData(0, Qt::UserRole, wnd->name);
        wndItem->setData(0, Qt::UserRole + 1, "window");
        m_tree->addTopLevelItem(wndItem);

        for (const auto& ctrl : wnd->controls) {
            if (!ctrl) continue;
            auto* ctrlItem = new QTreeWidgetItem({ ctrl->id });
            ctrlItem->setData(0, Qt::UserRole, wnd->name + "::" + ctrl->id);
            ctrlItem->setData(0, Qt::UserRole + 1, "control");
            wndItem->addChild(ctrlItem);
        }
    }

    m_tree->expandAll();
}

void WindowPanel::forceRefreshIfEmpty()
{
    if (m_tree->topLevelItemCount() == 0) {
        qInfo() << "[WindowPanel] Tree war leer, erzwungener Refresh...";
        updateWindowList();
    }
}

void WindowPanel::onSearchTextChanged(const QString& text)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* wndItem = m_tree->topLevelItem(i);
        bool wndVisible = wndItem->text(0).contains(text, Qt::CaseInsensitive);

        for (int j = 0; j < wndItem->childCount(); ++j) {
            auto* ctrl = wndItem->child(j);
            bool ctrlVisible = ctrl->text(0).contains(text, Qt::CaseInsensitive);
            ctrl->setHidden(!ctrlVisible && !wndVisible);
            if (ctrlVisible)
                wndVisible = true;
        }

        wndItem->setHidden(!wndVisible);
    }
}

void WindowPanel::onItemClicked(QTreeWidgetItem* item, int)
{
    if (!item) return;

    const QString type = item->data(0, Qt::UserRole + 1).toString();
    const QString idData = item->data(0, Qt::UserRole).toString();

    if (type == "control" && idData.contains("::")) {
        const auto parts = idData.split("::");
        emit controlSelected(parts[0], parts[1]);
        qDebug() << "[WindowPanel] Control selected:" << parts[0] << "::" << parts[1];
    } else if (type == "window") {
        emit windowSelected(idData);
        qDebug() << "[WindowPanel] Window selected:" << idData;
    }
}
