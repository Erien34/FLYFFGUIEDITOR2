#include "RenderControls.h"
#include <QDebug>

RenderControls::RenderControls(QObject* parent)
    : QObject(parent)
{
}

void RenderControls::render(QPainter& painter)
{
    painter.save();
    painter.setPen(Qt::green);
    painter.drawText(10, 60, "[RenderControls] aktiv");
    painter.restore();
}
