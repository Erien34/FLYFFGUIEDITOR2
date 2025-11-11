#include "RenderWindow.h"
#include "renderer/RenderControls.h"
#include "utils/ResourceUtils.h"
#include <QDebug>
#include <QFont>

namespace RenderWindow {

// =========================================================
// Fenster komplett zeichnen (ohne Controls)
// =========================================================
void renderWindow(QPainter& p,
                  const std::shared_ptr<WindowData>& wnd,
                  const QMap<QString, QPixmap>& themes,
                  const QSize& canvasSize)
{
    if (!wnd) return;

    QRect wndRect(0, 0, wnd->width, wnd->height);
    QPoint center(canvasSize.width()  / 2 - wndRect.width()  / 2,
                  canvasSize.height() / 2 - wndRect.height() / 2);
    wndRect.moveTopLeft(center);

    drawWindowTiles(p, wndRect, themes);

    // Fenster-Titel
    QRect titleBar = QRect(wndRect.left(), wndRect.top() + 4, wndRect.width(), 24);
    p.setPen(Qt::white);
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(titleBar, Qt::AlignHCenter | Qt::AlignVCenter, wnd->name);
}

// =========================================================
// Fensterrahmen (Tiles 00–11)
// =========================================================
void drawWindowTiles(QPainter& p, const QRect& area,
                     const QMap<QString, QPixmap>& themes)
{
    QString prefix = ResourceUtils::detectTilePrefix(themes, {"wndtile"}, "wndtile");
    if (prefix.isEmpty()) {
        qWarning() << "[RenderWindow] Kein Fenster-TileSet (wndtile) gefunden!";
        p.fillRect(area, QColor(80, 80, 80));
        return;
    }

    auto tile = [&](int i) -> QPixmap {
        QString key = QString("%1%2").arg(prefix).arg(i, 2, 10, QChar('0'));
        return themes.value(key);
    };

    QPixmap t00 = tile(0), t01 = tile(1), t02 = tile(2);
    QPixmap t03 = tile(3), t04 = tile(4), t05 = tile(5);
    QPixmap t06 = tile(6), t07 = tile(7), t08 = tile(8);
    QPixmap t09 = tile(9), t10 = tile(10), t11 = tile(11);

    constexpr qreal DEFAULT_WINDOW_ALPHA = 200.0 / 255.0;
    p.save();
    p.setOpacity(DEFAULT_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const int topH    = t01.height() + t04.height();
    const int leftW   = t06.width();
    const int rightW  = t08.width();
    const int bottomH = t10.height();

    QRect innerRect = area.adjusted(leftW, topH, -rightW, -bottomH + 1);
    if (!t07.isNull())
        p.drawTiledPixmap(innerRect, t07);
    else
        p.fillRect(innerRect, QColor(30, 30, 30));

    // Obere Reihe
    if (!t00.isNull()) p.drawPixmap(area.topLeft(), t00);
    if (!t01.isNull())
        p.drawTiledPixmap(QRect(area.left() + t00.width(), area.top(),
                                area.width() - t00.width() - t02.width(), t01.height()), t01);
    if (!t02.isNull()) p.drawPixmap(QPoint(area.right() - t02.width() + 1, area.top()), t02);

    // Header
    int headerY = area.top() + t01.height();
    if (!t03.isNull()) p.drawPixmap(QPoint(area.left(), headerY), t03);
    if (!t04.isNull())
        p.drawTiledPixmap(QRect(area.left() + t03.width(), headerY,
                                area.width() - t03.width() - t05.width(), t04.height()), t04);
    if (!t05.isNull()) p.drawPixmap(QPoint(area.right() - t05.width() + 1, headerY), t05);

    // Seiten
    int sideTop    = headerY + t04.height();
    int sideBottom = area.bottom() - bottomH + 1;
    int sideHeight = qMax(0, sideBottom - sideTop + 1);
    if (!t06.isNull())
        p.drawTiledPixmap(QRect(area.left(), sideTop, t06.width(), sideHeight), t06);
    if (!t08.isNull())
        p.drawTiledPixmap(QRect(area.right() - t08.width() + 1, sideTop,
                                t08.width(), sideHeight), t08);

    // Untere Reihe
    int bottomY = area.bottom() - bottomH + 1;
    if (!t09.isNull()) p.drawPixmap(QPoint(area.left(), bottomY), t09);
    if (!t10.isNull())
        p.drawTiledPixmap(QRect(area.left() + t09.width(), bottomY,
                                area.width() - t09.width() - t11.width(), t10.height()), t10);
    if (!t11.isNull())
        p.drawPixmap(QPoint(area.right() - t11.width() + 1, bottomY), t11);

    p.restore();
}

// =========================================================
// Client-Bereich (Fenster-Innenraum)
// =========================================================
QRect getClientRect(const std::shared_ptr<WindowData>& wnd,
                    const QMap<QString, QPixmap>& themes)
{
    QRect wndRect(0, 0, wnd->width, wnd->height);

    QString prefix = ResourceUtils::detectTilePrefix(themes, { "wndtile", "wnd" });
    int headerH = 0, bottomH = 0;

    if (!prefix.isEmpty()) {
        auto tile = [&](int idx) -> QPixmap {
            QString key = QString("%1%2").arg(prefix).arg(idx, 2, 10, QChar('0'));
            return themes.contains(key) ? themes.value(key) : QPixmap();
        };
        headerH = tile(5).height();
        bottomH = tile(10).height();
    }

    if (headerH == 0) headerH = 24;
    if (bottomH == 0) bottomH = 8;

    return wndRect.adjusted(12, headerH, -12, -bottomH);
}

} // namespace RenderWindow
