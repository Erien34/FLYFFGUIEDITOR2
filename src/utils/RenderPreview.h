#pragma once
#include <QPainter>
#include <QRect>
#include <QMap>
#include <QPixmap>
#include <QString>
#include "utils/ResourceUtils.h"
#include "utils/RenderControls.h"

namespace RenderPreview {

inline void drawButtonStatePreview(QPainter& p,
                                   const QRect& baseRect,
                                   const QMap<QString, QPixmap>& themes,
                                   const QString& prefix)
{
    const QList<int> offsets = {0, 10, 20, 30};
    const QStringList labels = {"Normal", "Pressed", "Hover", "Disabled"};
    const int spacing = 12;
    QRect rect = baseRect;

    for (int i = 0; i < offsets.size(); ++i) {
        QString testKey = QString("%1%2").arg(prefix).arg(offsets[i], 2, 10, QChar('0'));
        if (!themes.contains(testKey))
            continue;

        QRect previewRect(rect.left() + i * (rect.width() + spacing),
                          rect.top(),
                          rect.width(),
                          rect.height());

        // Hintergrund
        p.fillRect(previewRect, QColor(25, 25, 25));
        p.setPen(QColor(180, 180, 80));
        p.drawRect(previewRect.adjusted(0, 0, -1, -1));

        // 9-Slice mit Offset zeichnen
        QStringList candidate = { prefix };
        RenderControls::drawNineSlice(p, previewRect, themes, candidate);

        // Beschriftung
        p.setPen(Qt::white);
        QRect labelRect = previewRect.adjusted(0, previewRect.height() + 3, 0, 0);
        p.drawText(labelRect, Qt::AlignHCenter, labels[i]);
    }
}

} // namespace RenderPreview
