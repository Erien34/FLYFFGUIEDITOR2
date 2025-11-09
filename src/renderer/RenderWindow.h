#pragma once
#include <QPainter>
#include <QMap>
#include <QPixmap>
#include <memory>

#include "layout/model/WindowData.h"

namespace RenderWindow {

// Hauptfunktion: Zeichnet ein komplettes Fenster (Rahmen + Controls)

void renderWindow(QPainter& p,
                  const std::shared_ptr<WindowData>& wnd,
                  const QMap<QString, QPixmap>& themes,
                  const QSize& canvasSize);

// Zeichnet nur die Fenster-Tiles (Rahmen + Body)
void drawWindowTiles(QPainter& p, const QRect& area,
                     const QMap<QString, QPixmap>& themes);

// Zeichnet alle Controls im Fenster (unter Verwendung von RenderControls)
void drawWindowControls(QPainter& p, const std::shared_ptr<WindowData>& wnd,
                        const QMap<QString, QPixmap>& themes);

// Berechnet die Client-Area des Fensters (Bereich innerhalb des Rahmens)
QRect getClientRect(const std::shared_ptr<WindowData>& wnd,
                    const QMap<QString, QPixmap>& themes);

} // namespace RenderWindow
