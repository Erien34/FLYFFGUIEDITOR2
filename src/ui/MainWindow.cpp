#include "MainWindow.h"
#include "Canvas.h"
#include "core/ProjectController.h"
#include "ui/WindowPanel.h"
#include "ui/PropertyPanel.h"


#include <QSplitter>
#include <QVBoxLayout>
#include <QDebug>

MainWindow::MainWindow(ProjectController* controller, QWidget* parent)
    : QMainWindow(parent),
    m_controller(controller)
{
    qInfo() << "[MainWindow] Erzeuge Oberfläche...";

    // 🧩 Panels und Canvas erzeugen
    m_windowPanel   = new WindowPanel(controller, this);
    m_propertyPanel = new PropertyPanel(controller, this);
    m_canvasHandler = new CanvasHandler(controller, this);
    QWidget* canvasWidget = m_canvasHandler->canvas();

    // 🔹 Splitter-Layout für linke / mittlere / rechte Spalte
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_windowPanel);
    splitter->addWidget(canvasWidget);
    splitter->addWidget(m_propertyPanel);

    // 🔸 Canvas breiter als Panels
    splitter->setStretchFactor(1, 2);

    // 🔸 Standard-Startgrößen (wenn keine gespeicherten Werte existieren)
    splitter->setSizes({300, 900, 400});

    setCentralWidget(splitter);

    // 🔸 Mindestgröße + Standardgröße
    setMinimumSize(1200, 800);
    resize(1600, 900);

    // 🧱 Fenster-Layout-Einstellungen wiederherstellen
    QSettings settings("FlyFFTools", "FlyFFGUIEditor");
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    splitter->restoreState(settings.value("MainWindow/splitterState").toByteArray());

    qInfo() << "[MainWindow] Oberfläche initialisiert.";

    // 🔸 Splitter später speichern, wenn sich etwas ändert
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
    m_controller->bindCanvas(m_canvasHandler);

    // Falls bereits Layouts geladen sind, aktives Fenster auswählen
    auto lm = m_controller->layoutManager();
    if (!lm) return;

    const auto& windows = lm->processedWindows();
    if (!windows.empty() && windows.front()) {
        emit m_controller->activeWindowChanged(windows.front());
    }
}
