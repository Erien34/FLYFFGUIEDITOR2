#include "RenderWindow.h"
#include <QDebug>

RenderWindow::RenderWindow(QObject* parent)
    : QObject(parent)
{
}

void RenderWindow::render(QPainter& painter)
{
    painter.save();
    painter.setPen(Qt::yellow);
    painter.drawText(10, 40, "[RenderWindow] aktiv");
    painter.restore();
}
