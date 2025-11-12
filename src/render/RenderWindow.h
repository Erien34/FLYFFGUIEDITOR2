#pragma once
#include <QObject>
#include <QPainter>

class RenderWindow : public QObject {
    Q_OBJECT
public:
    explicit RenderWindow(QObject* parent = nullptr);
    void render(QPainter& painter);
};
