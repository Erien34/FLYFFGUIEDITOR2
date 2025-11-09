#pragma once
#pragma message(">>> Using RenderControls.h from: " __FILE__)
#include <QPainter>
#include <QRect>
#include <QMap>
#include <QPixmap>
#include <memory>
#include "model/ControlData.h"
#include "ResourceUtils.h"

namespace RenderControls
{
// =====================================================
//  🔹 Hilfsfunktion – Edit Background
// =====================================================
inline void renderEditBackground(QPainter& p, const QRect& rect,
                                 const QMap<QString, QPixmap>& themes)
{
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const QStringList candidates = { "wndedit", "wndedittile", "edit" };
    const QString prefix = ResourceUtils::detectTilePrefix(themes, candidates);
    if (prefix.isEmpty()) {
        // Fallback: dunkler Rahmen
        p.fillRect(rect, QColor(20, 20, 20));
        p.setPen(QColor(0, 0, 0, 150));
        p.drawRect(rect.adjusted(0, 0, -1, -1));
        p.restore();
        return;
    }

    auto getTile = [&](int idx) -> QPixmap {
        QString key = QString("%1%2").arg(prefix).arg(idx, 2, 10, QChar('0'));
        return themes.contains(key) ? themes.value(key) : QPixmap();
    };

    QPixmap tl = getTile(0);
    QPixmap tm = getTile(1);
    QPixmap tr = getTile(2);
    QPixmap ml = getTile(3);
    QPixmap mm = getTile(4);
    QPixmap mr = getTile(5);
    QPixmap bl = getTile(6);
    QPixmap bm = getTile(7);
    QPixmap br = getTile(8);

    int tileW = mm.width();
    int tileH = mm.height();

    // Mitte (gefüllt)
    for (int y = rect.top(); y < rect.bottom(); y += tileH)
        for (int x = rect.left(); x < rect.right(); x += tileW)
            p.drawPixmap(QRect(x, y, tileW, tileH), mm);

    // Kanten
    if (!tm.isNull()) p.drawTiledPixmap(QRect(rect.left() + tl.width(), rect.top(), rect.width() - tl.width() - tr.width(), tm.height()), tm);
    if (!bm.isNull()) p.drawTiledPixmap(QRect(rect.left() + bl.width(), rect.bottom() - bm.height(), rect.width() - bl.width() - br.width(), bm.height()), bm);
    if (!ml.isNull()) p.drawTiledPixmap(QRect(rect.left(), rect.top() + tl.height(), ml.width(), rect.height() - tl.height() - bl.height()), ml);
    if (!mr.isNull()) p.drawTiledPixmap(QRect(rect.right() - mr.width(), rect.top() + tr.height(), mr.width(), rect.height() - tr.height() - br.height()), mr);

    // Ecken
    if (!tl.isNull()) p.drawPixmap(rect.topLeft(), tl);
    if (!tr.isNull()) p.drawPixmap(rect.topRight() - QPoint(tr.width(), 0), tr);
    if (!bl.isNull()) p.drawPixmap(rect.bottomLeft() - QPoint(0, bl.height()), bl);
    if (!br.isNull()) p.drawPixmap(rect.bottomRight() - QPoint(br.width(), br.height()), br);

    p.restore();
}
// =====================================================
//  🔹 Hilfsfunktion – 9-Slice Rendering
// =====================================================
inline void drawNineSlice(QPainter& p, const QRect& rect,
                          const QMap<QString, QPixmap>& themes,
                          const QStringList& candidates)
{
    // Ermittelt Prefix
    QString prefix = ResourceUtils::detectTilePrefix(themes, candidates);
    if (prefix.isEmpty()) {
        p.fillRect(rect, QColor(80, 80, 80));
        p.setPen(QColor(200, 200, 200));
        p.drawRect(rect.adjusted(0, 0, -1, -1));
        return;
    }

    auto tile = [&](int i) -> QPixmap {
        QString key = QString("%1%2").arg(prefix).arg(i, 2, 10, QChar('0'));
        return themes.contains(key) ? themes.value(key) : QPixmap();
    };

    // Tiles laden
    QPixmap tl = tile(0), tm = tile(1), tr = tile(2);
    QPixmap ml = tile(3), mm = tile(4), mr = tile(5);
    QPixmap bl = tile(6), bm = tile(7), br = tile(8);

    // Wenn alle leer → graue Fläche als Notlösung
    if (tl.isNull() && tm.isNull() && tr.isNull() && ml.isNull() && mm.isNull()) {
        p.fillRect(rect, QColor(60, 60, 60));
        p.setPen(QColor(180, 180, 180));
        p.drawRect(rect.adjusted(0, 0, -1, -1));
        return;
    }

    const int edgeW = tl.isNull() ? 4 : tl.width();
    const int edgeH = tl.isNull() ? 4 : tl.height();

    // Center
    QRect center(rect.left() + edgeW,
                 rect.top() + edgeH,
                 rect.width() - 2 * edgeW,
                 rect.height() - 2 * edgeH);

    if (!mm.isNull())
        p.drawTiledPixmap(center, mm);
    else
        p.fillRect(center, QColor(50, 50, 50));

    // Top
    if (!tl.isNull()) p.drawPixmap(rect.topLeft(), tl);
    if (!tm.isNull()) p.drawTiledPixmap(QRect(rect.left() + edgeW, rect.top(),
                                rect.width() - 2 * edgeW, edgeH), tm);
    if (!tr.isNull()) p.drawPixmap(rect.topRight() - QPoint(tr.width(), 0), tr);

    // Middle left/right
    if (!ml.isNull()) p.drawTiledPixmap(QRect(rect.left(), rect.top() + edgeH,
                                edgeW, rect.height() - 2 * edgeH), ml);
    if (!mr.isNull()) p.drawTiledPixmap(QRect(rect.right() - edgeW, rect.top() + edgeH,
                                edgeW, rect.height() - 2 * edgeH), mr);

    // Bottom
    if (!bl.isNull()) p.drawPixmap(rect.bottomLeft() - QPoint(0, bl.height()), bl);
    if (!bm.isNull()) p.drawTiledPixmap(QRect(rect.left() + edgeW,
                                rect.bottom() - edgeH,
                                rect.width() - 2 * edgeW, edgeH), bm);
    if (!br.isNull()) p.drawPixmap(rect.bottomRight() - QPoint(br.width(), br.height()), br);
}

// =====================================================
//  🔹 Button State Extraction
// =====================================================
inline QPixmap extractButtonState(const QMap<QString, QPixmap>& themes,
                                  const QString& key,
                                  ButtonState state = Normal)
{
    if (!themes.contains(key))
        return QPixmap();

    QPixmap src = themes.value(key);
    if (src.isNull())
        return QPixmap();

    const int w = src.width();
    const int h = src.height();

    // Sprite-Sheet-Erkennung
    if (w > h * 1.5) {
        const int frameCount = 4; // Normal / Hover / Pressed / Disabled
        const int frameWidth = w / frameCount;
        state = std::clamp(state, Normal, Disabled);
        return src.copy(state * frameWidth, 0, frameWidth, h);
    }

    // Einzelbild-Fallback
    return src;
}

// =====================================================
//  🔹 Editbox Rendering (überdeckend, keine Lücken)
// =====================================================
inline void renderEdit(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes)
{
    Q_UNUSED(ctrl);

    const QStringList candidates = { "wndedit", "wndedittile", "edit" };
    const QString prefix = ResourceUtils::detectTilePrefix(themes, candidates);
    if (prefix.isEmpty()) return;

    auto getTile = [&](int idx) -> QPixmap {
        QString key = QString("%1%2").arg(prefix).arg(idx, 2, 10, QChar('0'));
        return themes.contains(key) ? themes.value(key) : QPixmap();
    };

    QPixmap tl = getTile(0);
    QPixmap tm = getTile(1);
    QPixmap tr = getTile(2);
    QPixmap bl = getTile(6);
    QPixmap bm = getTile(7);
    QPixmap br = getTile(8);

    const int edgeW = tl.width();
    const int edgeH = tl.height();

    // Mitte zuerst (volle Fläche)
    for (int y = rect.top(); y < rect.bottom(); y += tm.height()) {
        for (int x = rect.left(); x < rect.right(); x += tm.width()) {
            int w = std::min(tm.width(), rect.right() - x);
            int h = std::min(tm.height(), rect.bottom() - y);
            p.drawPixmap(QRect(x, y, w, h), tm, QRect(0, 0, w, h));
        }
    }

    // Oben / Unten
    for (int x = rect.left() + edgeW; x < rect.right() - edgeW; x += tm.width()) {
        int w = std::min(tm.width(), rect.right() - edgeW - x);
        p.drawPixmap(QRect(x, rect.top(), w, edgeH), tm, QRect(0, 0, w, edgeH));
    }
    if (!bm.isNull()) {
        for (int x = rect.left() + edgeW; x < rect.right() - edgeW; x += bm.width()) {
            int w = std::min(bm.width(), rect.right() - edgeW - x);
            p.drawPixmap(QRect(x, rect.bottom() - bm.height(), w, bm.height()),
                         bm, QRect(0, 0, w, bm.height()));
        }
    }

    // Seiten
    if (!bl.isNull()) {
        for (int y = rect.top() + edgeH; y < rect.bottom() - edgeH; y += bl.height()) {
            int h = std::min(bl.height(), rect.bottom() - edgeH - y);
            p.drawPixmap(QRect(rect.left(), y, edgeW, h),
                         bl, QRect(0, 0, edgeW, h));
        }
    }
    if (!br.isNull()) {
        for (int y = rect.top() + edgeH; y < rect.bottom() - edgeH; y += br.height()) {
            int h = std::min(br.height(), rect.bottom() - edgeH - y);
            p.drawPixmap(QRect(rect.right() - br.width(), y, br.width(), h),
                         br, QRect(0, 0, br.width(), h));
        }
    }

    // Ecken drüber
    p.drawPixmap(rect.topLeft(), tl);
    p.drawPixmap(rect.topRight() - QPoint(tr.width(), 0), tr);
    p.drawPixmap(rect.bottomLeft() - QPoint(0, bl.height()), bl);
    p.drawPixmap(rect.bottomRight() - QPoint(br.width(), br.height()), br);
}

// ================================================================
// Text Control (WTYPE_TEXT) – aktuell nur Hintergrund, kein Text
// ================================================================
inline void renderText(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes)
{
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] Text (placeholder):" << ctrl->id << "Texture:" << ctrl->texture;

    // Hintergrund wie bei Edit – gleiche Tiles, kein Text
    renderEditBackground(p, rect, themes);

    // Später: Textdarstellung
    // p.setPen(Qt::white);
    // p.drawText(rect, Qt::AlignCenter, ctrl->caption);
}



// =====================================================
//  🔹 Button-Renderer
// =====================================================

// Standard Buttons
inline void renderStandardButton(QPainter& p, const QRect& rect,
                                 const std::shared_ptr<ControlData>& ctrl,
                                 const QMap<QString, QPixmap>& themes)
{
    // 🔹 Transparenz & Blendmodus wie im FlyFF-Client
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] Button:" << ctrl->id << "Texture:" << ctrl->texture;

    // =====================================================
    // 1️⃣ Direkte Texturverwendung (wenn im Control angegeben)
    // =====================================================
    if (!ctrl->texture.isEmpty()) {
        QString foundKey = ResourceUtils::findTextureKey(themes, ctrl->texture);
        if (!foundKey.isEmpty()) {
            QPixmap tex = themes.value(foundKey);
            if (!tex.isNull()) {
                qDebug() << "[Render] Texture found:" << foundKey
                         << "(" << tex.width() << "x" << tex.height() << ")";

                int w = tex.width();
                int h = tex.height();
                const int slice = 6; // typischer FlyFF-Rand

                // 🔹 Multi-State-Erkennung (4 States nebeneinander)
                if (w >= h * 4) {
                    w /= 4; // nur den ersten State (Normal) verwenden
                    tex = tex.copy(0, 0, w, h);
                    qDebug() << "[Render] Multi-state texture detected, using first slice (" << w << "x" << h << ")";
                }

                // 🔹 3-Slice horizontaler Button (linke/mittlere/rechte Zone)
                if (w > slice * 2) {
                    QPixmap left  = tex.copy(0, 0, slice, h);
                    QPixmap mid   = tex.copy(slice, 0, w - slice * 2, h);
                    QPixmap right = tex.copy(w - slice, 0, slice, h);

                    // Zeichnen
                    p.drawPixmap(rect.left(), rect.top(), left);
                    p.drawTiledPixmap(QRect(rect.left() + slice, rect.top(),
                                            rect.width() - slice * 2, rect.height()), mid);
                    p.drawPixmap(rect.right() - slice + 1, rect.top(), right);
                } else {
                    // 🔹 Einfache Textur – direkt skalieren
                    p.drawPixmap(rect, tex.scaled(rect.size(),
                                                  Qt::IgnoreAspectRatio,
                                                  Qt::SmoothTransformation));
                }
                return; // ✔ Erfolgreich gerendert
            }
        }

        qDebug() << "[Render] Keine passende Textur gefunden, versuche Prefix...";
    }

    // =====================================================
    // 2️⃣ Fallback: Prefix-Suche (z. B. wndbutton / button)
    // =====================================================
    const QStringList candidates = { "wndbutton", "button" };
    const QString prefix = ResourceUtils::detectTilePrefix(themes, candidates);

    if (prefix.isEmpty()) {
        qDebug() << "[Render] Kein Prefix gefunden für Button – Fallback aktiv!";
        p.fillRect(rect, QColor(60, 60, 60)); // grauer Platzhalter
        return;
    }

    // =====================================================
    // 3️⃣ Standard-State laden (normal)
    // =====================================================
    QPixmap buttonPixmap = extractButtonState(themes, prefix + "00", Normal);
    if (!buttonPixmap.isNull()) {
        p.drawPixmap(rect, buttonPixmap.scaled(rect.size(),
                                               Qt::IgnoreAspectRatio,
                                               Qt::SmoothTransformation));
        qDebug() << "[Render] Prefix-basierten Button gezeichnet:" << prefix;
    } else {
        qDebug() << "[Render] Button-Fallback gezeichnet (fehlende Textur)";
        p.fillRect(rect, QColor(80, 80, 80));
    }
}

// ================================================================
// Checkboxes
// ================================================================
inline void renderCheckButton(QPainter& p, const QRect& rect,
                              const std::shared_ptr<ControlData>& ctrl,
                              const QMap<QString, QPixmap>& themes)
{
    // 🔹 Transparenz & Blendmodus wie im FlyFF-Client
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] CheckButton:" << ctrl->id << "Texture:" << ctrl->texture;

    QPixmap tex;

    // 1️⃣ Direkte Texturverwendung
    if (!ctrl->texture.isEmpty()) {
        QString foundKey = ResourceUtils::findTextureKey(themes, ctrl->texture);
        if (!foundKey.isEmpty())
            tex = themes.value(foundKey);
    }

    // 2️⃣ Fallback – falls Texture leer oder nicht gefunden
    if (tex.isNull()) {
        const QString prefix = ResourceUtils::detectTilePrefix(themes, { "buttcheck", "checkbox" });
        if (!prefix.isEmpty())
            tex = themes.value(prefix);
    }

    // 3️⃣ Kein Texture gefunden → Fallbackfarbe
    if (tex.isNull()) {
        qDebug() << "[Render] Keine CheckButton-Textur gefunden – Fallbackfarbe";
        p.fillRect(rect, QColor(80, 80, 80));
        return;
    }

    int w = tex.width();
    int h = tex.height();

    qDebug() << "[CheckButton Slice]" << "TexWidth:" << w << "TexHeight:" << h;

    // ==========================================================
    // 🔹 Bereichsauswahl: automatische Erkennung der Frames
    // ==========================================================
    if (w % 8 == 0 && h == 8 && w / 8 >= 1) {
        // Klassische 8×8-Frames
        int frames = w / 8;
        qDebug() << "[CheckButton Slice Detected]" << frames << "frames of 8x8";
        tex = tex.copy(0, 0, 8, 8);
    }
    else if (w % 6 == 0 && w / 6 == h) {
        // 6-State-Variante (z. B. 84×14)
        int frameWidth = w / 6;
        qDebug() << "[CheckButton 6-State Detected] FrameWidth:" << frameWidth;
        tex = tex.copy(0, 0, frameWidth, h);
    }
    else if (w > h * 4) {
        // 4-State-Variante
        int frameWidth = w / 4;
        qDebug() << "[CheckButton 4-State Detected] FrameWidth:" << frameWidth;
        tex = tex.copy(0, 0, frameWidth, h);
    }
    else {
        qDebug() << "[CheckButton Single Frame Detected]";
    }

    // ==========================================================
    // 🔹 Zentrierte Darstellung (max. 24×24 px, quadratisch)
    // ==========================================================
    const int size = std::min({ rect.width(), rect.height(), 24 });
    QRect target(rect.center().x() - size / 2,
                 rect.center().y() - size / 2,
                 size, size);

    p.drawPixmap(target, tex.scaled(size, size,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation));

    qDebug() << "[Render] CheckButton gezeichnet in" << target;
}


// ================================================================
// Radio Buttons
// ================================================================
inline void renderRadioButton(QPainter& p, const QRect& rect,
                              const std::shared_ptr<ControlData>& ctrl,
                              const QMap<QString, QPixmap>& themes)
{
    // 🔹 Transparenz
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] RadioButton:" << ctrl->id << "Texture:" << ctrl->texture;

    QPixmap tex;

    // 1️⃣ Direkte Texturverwendung
    if (!ctrl->texture.isEmpty()) {
        QString foundKey = ResourceUtils::findTextureKey(themes, ctrl->texture);
        if (!foundKey.isEmpty())
            tex = themes.value(foundKey);
    }

    // 2️⃣ Fallback per Prefix
    if (tex.isNull()) {
        const QString prefix = ResourceUtils::detectTilePrefix(themes, { "buttradio", "radiobutton" });
        if (!prefix.isEmpty())
            tex = themes.value(prefix);
    }

    if (tex.isNull()) {
        qDebug() << "[Render] Keine RadioButton-Textur gefunden – Fallbackfarbe";
        p.fillRect(rect, QColor(80, 80, 80));
        return;
    }

    int w = tex.width();
    int h = tex.height();

    qDebug() << "[RadioButton Slice]" << "TexWidth:" << w << "TexHeight:" << h;

    // ==========================================================
    // 🔹 Bereichsauswahl: automatische Erkennung der Frames
    // ==========================================================

    // Fall 1: 8×8-Frames (ältere FlyFF-Themes)
    if (w % 8 == 0 && h == 8 && w / 8 >= 1) {
        int frames = w / 8;
        qDebug() << "[RadioButton Slice Detected]" << frames << "frames of 8x8";
        tex = tex.copy(0, 0, 8, 8);
    }
    // Fall 2: 6 Zustände à 14×14 (dein Fall)
    else if (w % 6 == 0 && w / 6 == h) {
        int frameWidth = w / 6;
        qDebug() << "[RadioButton 6-State Detected] FrameWidth:" << frameWidth;
        tex = tex.copy(0, 0, frameWidth, h);
    }
    // Fall 3: Klassische 4-State-Textur (große Buttons)
    else if (w > h * 4) {
        int frameWidth = w / 4;
        qDebug() << "[RadioButton 4-State Detected] FrameWidth:" << frameWidth;
        tex = tex.copy(0, 0, frameWidth, h);
    }
    else {
        qDebug() << "[RadioButton Single Frame Detected]";
    }

    // ==========================================================
    // 🔹 Zentrierte Darstellung (max. 24×24 px, quadratisch)
    // ==========================================================
    const int size = std::min({ rect.width(), rect.height(), 24 });
    QRect target(rect.center().x() - size / 2,
                 rect.center().y() - size / 2,
                 size, size);

    p.drawPixmap(target, tex.scaled(size, size,
                                    Qt::KeepAspectRatio,
                                    Qt::SmoothTransformation));

    qDebug() << "[Render] RadioButton gezeichnet in" << target;
}


// ================================================================
// Static
// ================================================================

inline void renderStatic(QPainter& p, const QRect& rect,
                         const std::shared_ptr<ControlData>& ctrl,
                         const QMap<QString, QPixmap>& themes)
{
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] Static:" << ctrl->id << "Texture:" << ctrl->texture;

    QPixmap tex;

    // 1️⃣ Wenn das Static eine eigene Texture hat, versuch die zu laden
    if (!ctrl->texture.isEmpty()) {
        QString key = ResourceUtils::findTextureKey(themes, ctrl->texture);
        if (!key.isEmpty())
            tex = themes.value(key);
    }

    if (!tex.isNull()) {
        int w = tex.width();
        int h = tex.height();

        // Multi-State horizontal? → wie bei Buttons: nur ersten Frame (Normal) nehmen
        if (w > h * 1.5) {
            const int frameCount = 4;   // Normal / Hover / Pressed / Disabled
            const int frameWidth = w / frameCount;
            tex = tex.copy(0, 0, frameWidth, h);
        }

        // Bild einfach aufs Rect skalieren
        p.drawPixmap(rect, tex.scaled(rect.size(),
                                      Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation));
        return;
    }

    // 2️⃣ Kein Bild → einfacher Hintergrund + Rahmen als Platzhalter
    p.setOpacity(1.0);
    p.fillRect(rect, QColor(0, 0, 0, 80));                // leicht abgedunkelt
    p.setPen(QColor(255, 255, 255, 40));                  // sehr dezenter Rahmen
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    qDebug() << "[Render] Static without texture – placeholder frame drawn";
}

// ================================================================
// GroupBox
// ================================================================
inline void renderGroupBox(QPainter& p, const QRect& rect,
                           const std::shared_ptr<ControlData>& ctrl,
                           const QMap<QString, QPixmap>& themes)
{
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] GroupBox:" << ctrl->id << "Texture:" << ctrl->texture;

    // 1️⃣ Texturbasierte Darstellung (wenn vorhanden)
    if (!ctrl->texture.isEmpty()) {
        QString key = ResourceUtils::findTextureKey(themes, ctrl->texture);
        if (!key.isEmpty()) {
            const QPixmap& tex = themes[key];
            if (!tex.isNull()) {
                p.drawPixmap(rect, tex.scaled(rect.size(),
                                              Qt::IgnoreAspectRatio,
                                              Qt::SmoothTransformation));
                return;
            }
        }
    }

    // 2️⃣ Prefix-Suche nach groupbox
    const QStringList candidates = { "wndgroupbox", "groupbox" };
    QString prefix = ResourceUtils::detectTilePrefix(themes, candidates);
    if (!prefix.isEmpty()) {
        drawNineSlice(p, rect, themes, candidates);
        return;
    }

    // 3️⃣ Kein Theme → einfacher Rahmen
    p.setOpacity(1.0);
    p.fillRect(rect, QColor(30, 30, 30, 80));
    p.setPen(QColor(255, 255, 255, 40));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    // // Optional: Titel
    // if (!ctrl->caption.isEmpty()) {
    //     p.setPen(Qt::white);
    //     p.setFont(QFont("Arial", 10, QFont::Bold));
    //     QRect titleRect = QRect(rect.left() + 8, rect.top() + 2, rect.width() - 16, 20);
    //     p.drawText(titleRect, Qt::AlignLeft | Qt::AlignVCenter, ctrl->caption);
    // }
}
// ================================================================
// Combobox
// ================================================================
inline void renderComboBox(QPainter& p, const QRect& rect,
                           const std::shared_ptr<ControlData>& ctrl,
                           const QMap<QString, QPixmap>& themes)
{
    // 1️⃣ Hintergrund exakt wie EditCtrl, aber ohne Text
    renderEditBackground(p, rect, themes);

    // 2️⃣ Pfeil rechts
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    QString arrowKey = ResourceUtils::findTextureKey(themes, "buttcombodown");
    if (!arrowKey.isEmpty()) {
        QPixmap arrow = themes.value(arrowKey);
        if (!arrow.isNull()) {
            arrow = ResourceUtils::applyMagentaMask(arrow);

            const int btnW = arrow.width();
            const int btnH = arrow.height();

            QRect btnRect(rect.right() - btnW + 1,
                          rect.top() + (rect.height() - btnH) / 2,
                          btnW, btnH);

            p.drawPixmap(btnRect, arrow);
        }
    }

    p.restore();
}

// ================================================================
// Horizontaler TabCtrl (z. B. Party-Fenster)
// ================================================================
inline void renderHorizontalTabCtrl(QPainter& p, const QRect& rect,
                                    const std::shared_ptr<ControlData>& ctrl,
                                    const QMap<QString, QPixmap>& themes)
{
    Q_UNUSED(ctrl);

    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // ------------------------------------------------------------
    // 1️⃣ Prefix-Erkennung & Tab-Tiles laden
    // ------------------------------------------------------------
    const QString prefix = ResourceUtils::detectTilePrefix(
        themes, { "wndtabtile", "tabtile", "wndtab" });

    auto tile = [&](int idx) -> QPixmap {
        if (prefix.isEmpty()) return QPixmap();
        const QString key = QString("%1%2").arg(prefix).arg(idx, 2, 10, QChar('0'));
        return themes.value(key);
    };

    // Inaktive Tabs (00–02)
    QPixmap tabInactiveL = tile(0);
    QPixmap tabInactiveM = tile(1);
    QPixmap tabInactiveR = tile(2);

    // Aktive Tabs (10–12)
    QPixmap tabActiveL   = tile(10);
    QPixmap tabActiveM   = tile(11);
    QPixmap tabActiveR   = tile(12);

    // ------------------------------------------------------------
    // 2️⃣ Tab-Höhe bestimmen
    // ------------------------------------------------------------
    int tabHeight = 20;
    if (!tabActiveM.isNull())
        tabHeight = tabActiveM.height();
    else if (!tabInactiveM.isNull())
        tabHeight = tabInactiveM.height();

    tabHeight = std::clamp(tabHeight, 16, rect.height() / 3);

    const int tabCount = 2;  // Party: Information / Skill
    const int tabWidth = std::max(40, rect.width() / tabCount);

    // ------------------------------------------------------------
    // 3️⃣ Body zeichnen (WndEditTile bevorzugt)
    //    Body endet knapp über der Tab-Leiste, damit genau
    //    nur der Edit-Rand zu sehen ist – keine zweite Linie.
    // ------------------------------------------------------------
    QRect bodyRect = rect.adjusted(0, 0, 0, -tabHeight + 1);

    if (bodyRect.height() > 0) {
        renderEditBackground(p, bodyRect, themes);

        if (!themes.contains("wndedittile00") && !themes.contains("wndtile00"))
            p.fillRect(bodyRect, QColor(20, 20, 20));
    }

    // ------------------------------------------------------------
    // 4️⃣ Tab-Bar zeichnen (Hintergrund für Tabs)
    //    Startet direkt an der Body-Unterkante, überlappt um 1px.
    // ------------------------------------------------------------
    QRect barRect(rect.left(),
                  bodyRect.bottom() - 1,
                  rect.width(),
                  tabHeight + 1);

    if (!tabInactiveM.isNull())
        p.drawTiledPixmap(barRect, tabInactiveM);
    else
        p.fillRect(barRect, QColor(70, 60, 40)); // Fallback

    // ------------------------------------------------------------
    // 5️⃣ Tabs zeichnen (links aktiv, rechts inaktiv)
    // ------------------------------------------------------------
    QRect activeRect(barRect.left(), barRect.top(), tabWidth, tabHeight + 1);
    QRect inactiveRect(barRect.left() + tabWidth, barRect.top(),
                       tabWidth, tabHeight + 1);

    auto drawTab = [&](const QRect& target,
                       const QPixmap& left, const QPixmap& mid, const QPixmap& right)
    {
        int x = target.left();
        int y = target.top();

        if (!left.isNull()) {
            p.drawPixmap(x, y, left);
            x += left.width();
        }

        int rightEdge = target.right();
        if (!right.isNull())
            rightEdge -= right.width();

        if (!mid.isNull() && x <= rightEdge)
            p.drawTiledPixmap(QRect(x, y, rightEdge - x + 1, target.height()), mid);

        if (!right.isNull())
            p.drawPixmap(target.right() - right.width() + 1, y, right);
    };

    // Aktiver Tab
    drawTab(activeRect, tabActiveL, tabActiveM, tabActiveR);
    // Inaktiver Tab
    drawTab(inactiveRect, tabInactiveL, tabInactiveM, tabInactiveR);

    // ------------------------------------------------------------
    // 6️⃣ Platzhalter-Texte (Textsystem später)
    // ------------------------------------------------------------
    QFont fontActive("Arial", 8, QFont::Bold);
    QFont fontInactive("Arial", 8);
    p.setFont(fontActive);
    p.setPen(Qt::white);
    p.drawText(activeRect, Qt::AlignCenter, "Information");

    p.setFont(fontInactive);
    p.setPen(QColor(180, 160, 130));
    p.drawText(inactiveRect, Qt::AlignCenter, "Skill");

    p.restore();
}

inline void renderVerticalTabCtrl(QPainter& p, const QRect& rect,
                                  const std::shared_ptr<ControlData>& ctrl,
                                  const QMap<QString, QPixmap>& themes)
{
    Q_UNUSED(ctrl);

    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // ------------------------------------------------------------
    // Helper-Funktionen
    // ------------------------------------------------------------
    auto getState = [](const QPixmap& src, int stateIndex = 0) -> QPixmap {
        if (src.isNull()) return src;

        const int w = src.width();
        const int h = src.height();
        const int frameCount = 4;

        if (w >= h * 2) {
            const int frameWidth = w / frameCount;
            stateIndex = std::clamp(stateIndex, 0, frameCount - 1);
            return src.copy(stateIndex * frameWidth, 0, frameWidth, h);
        }
        if (h >= w * 2) {
            const int frameHeight = h / frameCount;
            stateIndex = std::clamp(stateIndex, 0, frameCount - 1);
            return src.copy(0, stateIndex * frameHeight, w, frameHeight);
        }
        return src;
    };

    auto loadTex = [&](const QString& name) -> QPixmap {
        QString key = ResourceUtils::findTextureKey(themes, name);
        if (key.isEmpty()) return QPixmap();
        return themes.value(key);
    };

    // ------------------------------------------------------------
    // Tab-Tiles unten (WndTabTile10–12, Abschluss = 15)
    // ------------------------------------------------------------
    const QString tabPrefix = ResourceUtils::detectTilePrefix(
        themes, { "wndtabtile", "tabtile", "wndtab" });

    auto tabTile = [&](int idx) -> QPixmap {
        if (tabPrefix.isEmpty()) return QPixmap();
        const QString key = QString("%1%2").arg(tabPrefix).arg(idx, 2, 10, QChar('0'));
        return themes.contains(key) ? themes.value(key) : QPixmap();
    };

    QPixmap tabActiveL = tabTile(10);
    QPixmap tabActiveM = tabTile(11);
    QPixmap tabActiveR = tabTile(12);
    QPixmap tabEndTile = tabTile(15);

    int tabHeight = 0;
    if (!tabActiveM.isNull()) tabHeight = tabActiveM.height();
    else if (!tabActiveL.isNull()) tabHeight = tabActiveL.height();
    if (tabHeight <= 0) tabHeight = 18;

    // ------------------------------------------------------------
    // Geometrie
    // ------------------------------------------------------------
    QRect contentRect = rect.adjusted(0, 0, 0, -tabHeight);
    contentRect = contentRect.normalized();

    // Scrollbar-Grafiken
    QPixmap texUp   = getState(loadTex("buttvscrup"),   0);
    QPixmap texDown = getState(loadTex("buttvscrdown"), 0);
    QPixmap texBar  = getState(loadTex("buttvscrbar"),  1);

    texUp   = ResourceUtils::applyMagentaMask(texUp);
    texDown = ResourceUtils::applyMagentaMask(texDown);
    texBar  = ResourceUtils::applyMagentaMask(texBar);

    int scrollWidth = 16;
    if (!texUp.isNull())        scrollWidth = texUp.width();
    else if (!texDown.isNull()) scrollWidth = texDown.width();
    else if (!texBar.isNull())  scrollWidth = texBar.width();
    scrollWidth = std::min(scrollWidth, contentRect.width() / 4);

    QRect bodyRect = contentRect.adjusted(0, 0, -scrollWidth, 0);

    // ------------------------------------------------------------
    // Body (linker Inhaltsbereich)
    // ------------------------------------------------------------
    if (bodyRect.width() > 0) {
        renderEditBackground(p, bodyRect, themes);
        if (!themes.contains("wndedittile00") && !themes.contains("wndtile00"))
            p.fillRect(bodyRect, QColor(15, 15, 15));
    }

    // ------------------------------------------------------------
    // 5️⃣ Unterer Tab-Header (mit rechtem Abschluss-Tile 15)
    // ------------------------------------------------------------
    {
        // Tab-Leiste leicht überlappend links und bis ganz rechts
        QRect tabRect(bodyRect.left() - 1,
                      contentRect.bottom() + 1,
                      bodyRect.width() + 1,
                      tabHeight);

        auto drawTab = [&](const QRect& target,
                           const QPixmap& left, const QPixmap& mid, const QPixmap& right)
        {
            int x = target.left();
            int y = target.top();

            // linker Startteil (rund)
            if (!left.isNull()) {
                p.drawPixmap(x, y, left);
                x += left.width();
            }

            // rechter Bereich abziehen, um Platz für right + endtile zu lassen
            int rightEdge = target.right();
            if (!right.isNull())
                rightEdge -= right.width();

            // mittleres Tile kacheln
            if (!mid.isNull() && x <= rightEdge)
                p.drawTiledPixmap(QRect(x, y, rightEdge - x + 1, target.height()), mid);

            // rechter Tabteil
            if (!right.isNull())
                p.drawPixmap(rightEdge + 1, y, right);
        };

        if (!tabActiveM.isNull()) {
            drawTab(tabRect, tabActiveL, tabActiveM, tabActiveR);

            // rechter Abschluss (WndTabTile15)
            if (!tabEndTile.isNull()) {
                tabEndTile = ResourceUtils::applyMagentaMask(tabEndTile);
                int x = tabRect.right() + 1; // direkt nach Tile12
                int y = tabRect.top();
                p.drawPixmap(x, y, tabEndTile);
            }

            // Tabtext
            p.setPen(Qt::white);
            QFont font("Arial", 9);
            p.setFont(font);
            p.drawText(tabRect.adjusted(6, 0, -4, 0),
                       Qt::AlignVCenter | Qt::AlignLeft,
                       "Item");
        } else {
            // Fallback
            p.fillRect(tabRect, QColor(25, 25, 25));
            p.setPen(QColor(0, 0, 0, 160));
            p.drawLine(tabRect.left(), tabRect.top(), tabRect.right(), tabRect.top());
        }
    }

    // ------------------------------------------------------------
    // Scrollbar (über Tab)
    // ------------------------------------------------------------
    QRect scrollRect(contentRect.right() - scrollWidth + 1,
                     contentRect.top(),
                     scrollWidth,
                     contentRect.height());

    const int upHeight    = !texUp.isNull()   ? texUp.height()   : 16;
    const int downHeight  = !texDown.isNull() ? texDown.height() : upHeight;
    const int trackHeight = std::max(0, scrollRect.height() - upHeight - downHeight - 1);

    QRect upRect(scrollRect.left(), scrollRect.top(), scrollRect.width(), upHeight);
    QRect downRect(scrollRect.left(),
                   scrollRect.bottom() - downHeight + 1,
                   scrollRect.width(),
                   downHeight);
    QRect trackRect(scrollRect.left(),
                    upRect.bottom() + 1,
                    scrollRect.width(),
                    trackHeight);

    // Scroll-Track
    if (trackRect.height() > 0) {
        if (!texBar.isNull()) {
            const int visibleWidth = qMin(6, texBar.width());
            const int barX = trackRect.left() + (trackRect.width() - visibleWidth) / 2;

            for (int y = trackRect.top(); y < trackRect.bottom(); y += texBar.height()) {
                const int h = std::min(texBar.height(), trackRect.bottom() - y);
                QRect target(barX, y, visibleWidth, h);
                QRect source((texBar.width() - visibleWidth) / 2, 0, visibleWidth, h);
                p.drawPixmap(target, texBar, source);
            }
        } else {
            p.fillRect(trackRect, QColor(45, 45, 45));
        }
    }

    // Up-/Down-Buttons
    if (!texUp.isNull())
        p.drawPixmap(upRect, texUp.scaled(upRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    if (!texDown.isNull())
        p.drawPixmap(downRect, texDown.scaled(downRect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

    // Lichtkante rechts
    p.setPen(QColor(255, 255, 255, 50));
    p.drawLine(scrollRect.right(), scrollRect.top(), scrollRect.right(), scrollRect.bottom());

    p.restore();
}

// ------------------------------------------------
//  VERTICAL TAB CONTAINER (z. B. Guild Bank, Inventory)
// ------------------------------------------------
inline void renderVerticalTabContainer(QPainter& p, const QRect& rect,
                                       const std::shared_ptr<ControlData>& ctrl,
                                       const QMap<QString, QPixmap>& themes)
{
    constexpr qreal alpha = 200.0 / 255.0;
    p.setOpacity(alpha);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] VerticalTabContainer:" << ctrl->id;

    // Hintergrund (meist wndlistctrl oder wndtabtile)
    const QStringList candidates = { "wndlistctrl", "wndtabtile", "wndlistbox" };
    drawNineSlice(p, rect, themes, candidates);

    // Visuelle Scrollbar rechts
    const int scrollBarWidth = 12;
    QRect scrollBar(rect.right() - scrollBarWidth - 2,
                    rect.top() + 6,
                    scrollBarWidth,
                    rect.height() - 12);

    drawNineSlice(p, scrollBar, themes, { "wndscrollbar" });

    QString upKey = ResourceUtils::findTextureKey(themes, "wndscrollup");
    QString downKey = ResourceUtils::findTextureKey(themes, "wndscrolldown");
    QString thumbKey = ResourceUtils::findTextureKey(themes, "wndscrollthumb");

    const int arrowSize = 10;
    QRect upRect(scrollBar.left(), scrollBar.top(), scrollBar.width(), arrowSize);
    QRect downRect(scrollBar.left(), scrollBar.bottom() - arrowSize,
                   scrollBar.width(), arrowSize);
    QRect thumbRect(scrollBar.left() + 1,
                    scrollBar.center().y() - 12,
                    scrollBar.width() - 2,
                    24);

    if (!upKey.isEmpty() && themes.contains(upKey))
        p.drawPixmap(upRect, themes[upKey].scaled(upRect.size(),
                                                  Qt::IgnoreAspectRatio,
                                                  Qt::SmoothTransformation));
    else {
        p.fillRect(upRect, QColor(60, 60, 60));
        p.setPen(Qt::black);
        p.drawRect(upRect.adjusted(0, 0, -1, -1));
    }

    if (!downKey.isEmpty() && themes.contains(downKey))
        p.drawPixmap(downRect, themes[downKey].scaled(downRect.size(),
                                                      Qt::IgnoreAspectRatio,
                                                      Qt::SmoothTransformation));
    else {
        p.fillRect(downRect, QColor(60, 60, 60));
        p.setPen(Qt::black);
        p.drawRect(downRect.adjusted(0, 0, -1, -1));
    }

    if (!thumbKey.isEmpty() && themes.contains(thumbKey))
        p.drawPixmap(thumbRect, themes[thumbKey].scaled(thumbRect.size(),
                                                        Qt::IgnoreAspectRatio,
                                                        Qt::SmoothTransformation));
    else {
        p.fillRect(thumbRect, QColor(85, 85, 85));
        p.setPen(QColor(140, 140, 140));
        p.drawRect(thumbRect.adjusted(0, 0, -1, -1));
    }
}


// ================================================================
// Tab Control – Automatische Variante (horizontal / vertikal)
// ================================================================
inline void renderTabCtrl(QPainter& p, const QRect& rect,
                          const std::shared_ptr<ControlData>& ctrl,
                          const QMap<QString, QPixmap>& themes)
{
    bool isVertical = rect.height() > rect.width();
    if (isVertical)
        renderVerticalTabCtrl(p, rect, ctrl, themes);
    else
        renderHorizontalTabCtrl(p, rect, ctrl, themes);
}

// ================================================================
// ListBox
// ================================================================
inline void renderListBox(QPainter& p, const QRect& rect,
                          const std::shared_ptr<ControlData>& ctrl,
                          const QMap<QString, QPixmap>& themes)
{
    constexpr qreal FLYFF_WINDOW_ALPHA = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] ListBox:" << ctrl->id << "Texture:" << ctrl->texture;

    // 1️⃣ Texturbasierte Darstellung (wenn vorhanden)
    if (!ctrl->texture.isEmpty()) {
        QString key = ResourceUtils::findTextureKey(themes, ctrl->texture);
        if (!key.isEmpty()) {
            const QPixmap& tex = themes[key];
            if (!tex.isNull()) {
                p.drawPixmap(rect, tex.scaled(rect.size(),
                                              Qt::IgnoreAspectRatio,
                                              Qt::SmoothTransformation));
                return;
            }
        }
    }

    // 2️⃣ Prefix-Suche nach listbox
    const QStringList candidates = { "wndlistbox", "listbox" };
    QString prefix = ResourceUtils::detectTilePrefix(themes, candidates);
    if (!prefix.isEmpty()) {
        drawNineSlice(p, rect, themes, candidates);
        return;
    }

    // 3️⃣ Kein Theme → Standard-Fallback (leicht dunkler Hintergrund)
    p.setOpacity(1.0);
    p.fillRect(rect, QColor(20, 20, 20, 100));
    p.setPen(QColor(255, 255, 255, 25));
    p.drawRect(rect.adjusted(0, 0, -1, -1));
}


// =====================================================
//  🔹 Haupt-Renderer: renderControl()
// =====================================================
inline void renderControl(QPainter& p, const QRect& rect,
                          const std::shared_ptr<ControlData>& ctrl,
                          const QMap<QString, QPixmap>& themes)
{
    if (!ctrl) return;

    const QString type   = ctrl->type.toUpper();
    const QString texKey = ctrl->texture.toLower();

    qDebug() << "[RenderControl]" << ctrl->id << "Type:" << type << "Texture:" << texKey;

    // 1️⃣ Edit – Speziallogik
    if (type.contains("EDIT")) {
        renderEdit(p, rect, ctrl, themes);
        return;
    }

    // 2️⃣ Buttons
    if (type.contains("BUTTON")) {

        // ✅ Check-Button
        if (texKey.contains("buttcheck") || type.contains("CHECK")) {
            renderCheckButton(p, rect, ctrl, themes);
            return;
        }

        // ✅ Radio-Button
        if (texKey.contains("buttradio") || type.contains("RADIO")) {
            renderRadioButton(p, rect, ctrl, themes);
            return;
        }

        // ✅ Normaler Button
        renderStandardButton(p, rect, ctrl, themes);
        return;
    }

    // Static
    if (type.contains("STATIC")) {
        renderStatic(p, rect, ctrl, themes);
        return;
    }

    // Text
    if (type.contains("TEXT")) {
        renderText(p, rect, ctrl, themes);
        return;
    }

    // 2️⃣ Tab Controls
    if (type.contains("TABCTRL")) {
        // sehr einfache Heuristik:
        // breit -> horizontal, schmal & hoch -> vertikal
        if (rect.width() >= rect.height())
            renderHorizontalTabCtrl(p, rect, ctrl, themes);
        else
            renderVerticalTabCtrl(p, rect, ctrl, themes);
        return;
    }
    // 4️⃣ Listbox
    if (type.contains("LISTBOX")) {
        const QStringList candidates = { "wndlistbox", "listbox" };
        drawNineSlice(p, rect, themes, candidates);
        return;
    }

    // 5️⃣ Groupbox
    if (type.contains("GROUPBOX")) {
        const QStringList candidates = { "wndgroupbox", "groupbox" };
        drawNineSlice(p, rect, themes, candidates);
        return;
    }

    // 3️⃣ ComboBox
    if (type.contains("COMBO")) {
        renderComboBox(p, rect, ctrl, themes);
        return;
    }

    // 6️⃣ Fallback: direkte Textur (wenn angegeben)
    if (!texKey.isEmpty()) {
        QString key = ResourceUtils::findTextureKey(themes, texKey);
        if (!key.isEmpty()) {
            const QPixmap& pix = themes[key];
            p.drawPixmap(rect.topLeft(),
                         pix.scaled(rect.size(),
                                    Qt::IgnoreAspectRatio,
                                    Qt::SmoothTransformation));
            return;
        }
    }

    // 7️⃣ Letzter Fallback (rot umrahmt)
    p.setPen(Qt::red);
    p.drawRect(rect.adjusted(0, 0, -1, -1));
}

} // namespace RenderControls
