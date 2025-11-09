#include <QApplication>
#include <QDebug>
#include "core/ProjectController.h"
#include "ui/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("FlyFF GUI Editor");

    qDebug() << "[Main] Starte FlyFF GUI Editor...";

    qInstallMessageHandler([](QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
        // Nur relevante Logs durchlassen:
        if (msg.startsWith("[Render]") || msg.startsWith("[ResourceUtils]") || msg.startsWith("[Themes geladen]") || msg.startsWith("[QObject::]"))
            return; // 🚫 diese Zeilen unterdrücken

        // Standard-Ausgabe behalten
        QByteArray localMsg = msg.toLocal8Bit();
        fprintf(stderr, "%s\n", localMsg.constData());
    });

    ProjectController controller;
    MainWindow window(&controller);
    window.show();
    qDebug() << "[Main] MainWindow erfolgreich erstellt.";

    // Projekt laden
    if (!controller.loadProject("")) {
        qWarning() << "[Main] Projekt konnte nicht geladen werden.";
        return 1;
    }

    // Panels erst jetzt verbinden!
    window.initializeAfterLoad();

    qDebug() << "[Main] Event-Loop gestartet.";
    return app.exec();
}

