#pragma once
#include <QObject>
#include <QPainter>

class RenderControls : public QObject {
    Q_OBJECT
public:
    explicit RenderControls(QObject* parent = nullptr);
    void render(QPainter& painter);
};
