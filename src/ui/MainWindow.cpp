#include "MainWindow.h"
#include "Canvas.h"
#include "CanvasHandler.h"
#include "WindowPanel.h"
#include "PropertyPanel.h"
#include "ProjectController.h"
#include <QSplitter>
#include <QSettings>
#include <QDebug>

MainWindow::MainWindow(ProjectController* controller, QWidget* parent)
    : QMainWindow(parent),
    m_controller(controller)
{
    qInfo() << "[MainWindow] Erzeuge Oberfläche...";

    // Canvas und Handler
    m_canvas = new Canvas(controller, this);
    m_handler = new CanvasHandler(m_canvas, m_controller);

    QWidget* canvasWidget = m_handler->canvas();

    // Panels
    m_windowPanel   = new WindowPanel(controller, this);
    m_propertyPanel = new PropertyPanel(controller, this);

    // Layout
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_windowPanel);
    splitter->addWidget(canvasWidget);
    splitter->addWidget(m_propertyPanel);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({300, 900, 400});

    setCentralWidget(splitter);
    setMinimumSize(1200, 800);
    resize(1600, 900);

    // Layout wiederherstellen
    QSettings settings("FlyFFTools", "FlyFFGUIEditor");
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    splitter->restoreState(settings.value("MainWindow/splitterState").toByteArray());

    qInfo() << "[MainWindow] Oberfläche initialisiert.";

    connect(splitter, &QSplitter::splitterMoved, this, [splitter]() {
        QSettings settings("FlyFFTools", "FlyFFGUIEditor");
        settings.setValue("MainWindow/splitterState", splitter->saveState());
    });
}

void MainWindow::initializeAfterLoad()
{
    qInfo() << "[MainWindow] Controller-Bindings nach Projekt-Load aktiviert.";

    if (!m_controller)
        return;

    // Panels mit Controller verbinden
    m_controller->bindPanels(m_windowPanel, m_propertyPanel);
    m_windowPanel->updateWindowList();

    // Canvas-Logik mit Controller verbinden
    m_controller->bindCanvas(m_handler);

    // Falls bereits Layouts geladen sind, aktives Fenster auswählen
    auto lm = m_controller->layoutManager();
    if (!lm) return;

    const auto& windows = lm->processedWindows();
    if (!windows.empty() && windows.front()) {
        emit m_controller->activeWindowChanged(windows.front());
    }
}
