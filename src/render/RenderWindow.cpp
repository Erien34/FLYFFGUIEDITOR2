#include "RenderWindow.h"
#include <QDebug>

RenderWindow::RenderWindow(ThemeManager* themeManager,
                           BehaviorManager* behaviorManager,
                           QObject* parent)
    : QObject(parent),
    m_themeManager(themeManager),
    m_behaviorManager(behaviorManager)
{
}

void RenderWindow::render(QPainter* painter, const WindowData& window)
{
    if (!painter || !m_themeManager || !m_behaviorManager)
        return;

    // Verhalten (Styleflags) auflösen
    WindowData wndCopy = window;
    m_behaviorManager->applyWindowStyle(wndCopy);
    const QVector<QString>& flags = wndCopy.resolvedMask;

    // Rahmen zeichnen
    QRect frameRect(0, 0, window.width, window.height);
    drawFrame(painter, frameRect);

    // Titelbar-Bereich
    const int buttonSize = 20;
    const int spacing = 2;
    const int padding = 5;
    const int top = frameRect.top() + padding;
    int right = frameRect.right() - padding - buttonSize;

    // Reihenfolge: Close, Maximize, Minimize, Help, Extension, Pin, View
    struct ButtonInfo { QString flag; QString key; QColor fallback; };
    QList<ButtonInfo> buttons = {
                                 { "has_close",     "buttwndexit",      QColor(220, 60, 60) },
                                 { "has_maximize",  "buttwndmax",       QColor(60, 140, 220) },
                                 { "has_minimize",  "buttwndmin",       QColor(60, 180, 120) },
                                 { "has_help",      "buttwndhelp",      QColor(200, 200, 80) },
                                 { "has_extension", "buttwndextension", QColor(180, 120, 220) },
                                 { "has_pin",       "buttwndpin",       QColor(160, 160, 160) },
                                 { "has_view",      "buttwndview",      QColor(120, 200, 200) },
                                 };

    for (const auto& b : buttons)
    {
        if (!flags.contains(b.flag))
            continue;

        QRect rect(right, top, buttonSize, buttonSize);

        const QPixmap& texRef = m_themeManager->texture(b.key, ControlState::Normal);

        if (texRef.isNull()) {
            qWarning().noquote() << "[RenderWindow] Fallback für fehlende Textur:" << b.key;
            painter->setBrush(b.fallback);
            painter->setPen(Qt::NoPen);
            painter->drawRect(rect);
        } else {
            QPixmap tex = texRef;  // lokale Kopie für Lebenszeit-Sicherheit
            painter->drawPixmap(rect, tex);
        }

        right -= (buttonSize + spacing);
    }

    // Optional: Titeltext zeichnen, falls vorhanden
    if (flags.contains("has_caption")) {
        painter->setPen(Qt::lightGray);
        QRect titleRect = frameRect.adjusted(8, 0, -8, 0);
        painter->drawText(titleRect, Qt::AlignVCenter | Qt::AlignLeft, wndCopy.name);
    }
}


void RenderWindow::drawFrame(QPainter* painter, const QRect& rect)
{
    painter->save();
    painter->setPen(Qt::black);
    painter->setBrush(QColor(80, 80, 90, 180));
    painter->drawRect(rect);
    painter->restore();
}

void RenderWindow::drawCloseButton(QPainter* painter, const QRect& rect)
{
    const QPixmap& tex = m_themeManager->texture("ButtWndExit", ControlState::Normal);
    if (!tex.isNull())
        painter->drawPixmap(rect, tex);
    else
        painter->fillRect(rect, Qt::red);
}

void RenderWindow::drawHelpButton(QPainter* painter, const QRect& rect)
{
    const QPixmap& tex = m_themeManager->texture("ButtWndHelp", ControlState::Normal);
    if (!tex.isNull())
        painter->drawPixmap(rect, tex);
    else
        painter->fillRect(rect, Qt::blue);
}
