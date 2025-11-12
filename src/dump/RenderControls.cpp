#include "renderer/RenderControls.h"
#include "ResourceUtils.h"

#include <QDebug>
#include <QFont>
#include <algorithm>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(logRender, "render")

namespace RenderControls
{

// =====================================================
//  🔹 Hilfsfunktion – Edit Background
// =====================================================
void renderEditBackground(QPainter& p, const QRect& rect,
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

    int tileW = mm.isNull() ? 4 : mm.width();
    int tileH = mm.isNull() ? 4 : mm.height();

    // Mitte (gefüllt)
    for (int y = rect.top(); y < rect.bottom(); y += tileH)
        for (int x = rect.left(); x < rect.right(); x += tileW)
            p.drawPixmap(QRect(x, y, tileW, tileH), mm);

    // Kanten
    if (!tm.isNull())
        p.drawTiledPixmap(QRect(rect.left() + tl.width(), rect.top(),
                                rect.width() - tl.width() - tr.width(), tm.height()), tm);
    if (!bm.isNull())
        p.drawTiledPixmap(QRect(rect.left() + bl.width(), rect.bottom() - bm.height(),
                                rect.width() - bl.width() - br.width(), bm.height()), bm);
    if (!ml.isNull())
        p.drawTiledPixmap(QRect(rect.left(), rect.top() + tl.height(),
                                ml.width(), rect.height() - tl.height() - bl.height()), ml);
    if (!mr.isNull())
        p.drawTiledPixmap(QRect(rect.right() - mr.width(), rect.top() + tr.height(),
                                mr.width(), rect.height() - tr.height() - br.height()), mr);

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
void drawNineSlice(QPainter& p, const QRect& rect,
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
    if (!tm.isNull())
        p.drawTiledPixmap(QRect(rect.left() + edgeW, rect.top(),
                                rect.width() - 2 * edgeW, edgeH), tm);
    if (!tr.isNull()) p.drawPixmap(rect.topRight() - QPoint(tr.width(), 0), tr);

    // Middle left/right
    if (!ml.isNull())
        p.drawTiledPixmap(QRect(rect.left(), rect.top() + edgeH,
                                edgeW, rect.height() - 2 * edgeH), ml);
    if (!mr.isNull())
        p.drawTiledPixmap(QRect(rect.right() - edgeW, rect.top() + edgeH,
                                edgeW, rect.height() - 2 * edgeH), mr);

    // Bottom
    if (!bl.isNull()) p.drawPixmap(rect.bottomLeft() - QPoint(0, bl.height()), bl);
    if (!bm.isNull())
        p.drawTiledPixmap(QRect(rect.left() + edgeW,
                                rect.bottom() - edgeH,
                                rect.width() - 2 * edgeW, edgeH), bm);
    if (!br.isNull()) p.drawPixmap(rect.bottomRight() - QPoint(br.width(), br.height()), br);
}

// ======================================================
// Hilfsfunktion: Control-State-Texturen laden (FlyFF-Stil)
// ======================================================
// ======================================================
// Hilfsfunktion: Textur-Stages aus Spritesheet extrahieren
// ======================================================
static TextureStates loadTextureStages(const QMap<QString, QPixmap>& themes,
                                       const QString& baseKey)
{
    TextureStates stages;

    QString key = ResourceUtils::findTextureKey(themes, baseKey);
    if (key.isEmpty() || !themes.contains(key))
        return stages;

    const QPixmap& src = themes[key];
    if (src.isNull())
        return stages;

    const int w = src.width();
    const int h = src.height();

    // 🔹 4 States nebeneinander (Normal / Hover / Pressed / Disabled)
    if (w >= h * 4) {
        const int frameW = w / 4;
        stages.normal   = src.copy(0 * frameW, 0, frameW, h);
        stages.hover    = src.copy(1 * frameW, 0, frameW, h);
        stages.pressed  = src.copy(2 * frameW, 0, frameW, h);
        stages.disabled = src.copy(3 * frameW, 0, frameW, h);
    }
    // 🔹 4 States untereinander (vertikal)
    else if (h >= w * 4) {
        const int frameH = h / 4;
        stages.normal   = src.copy(0, 0 * frameH, w, frameH);
        stages.hover    = src.copy(0, 1 * frameH, w, frameH);
        stages.pressed  = src.copy(0, 2 * frameH, w, frameH);
        stages.disabled = src.copy(0, 3 * frameH, w, frameH);
    }
    else {
        // 🔹 Einzelbild → auf alle States anwenden
        stages.normal   = src;
        stages.hover    = src;
        stages.pressed  = src;
        stages.disabled = src;
    }

    return stages;
}

// ======================================================
// Vertikale Scrollbar rendern (FlyFF-Stil)
// ======================================================
void renderVerticalScrollBar(QPainter& p, const QRect& rect,
                             const QMap<QString, QPixmap>& themes)
{
    auto getState = [](const QPixmap& src, int stateIndex = 0) -> QPixmap {
        if (src.isNull()) return src;
        const int w = src.width();
        const int h = src.height();
        const int frameCount = 4;
        if (w >= h * frameCount) {
            const int frameW = w / frameCount;
            stateIndex = std::clamp(stateIndex, 0, frameCount - 1);
            return src.copy(stateIndex * frameW, 0, frameW, h);
        }
        if (h >= w * frameCount) {
            const int frameH = h / frameCount;
            stateIndex = std::clamp(stateIndex, 0, frameCount - 1);
            return src.copy(0, stateIndex * frameH, w, frameH);
        }
        return src;
    };

    auto loadTex = [&](const QString& name) -> QPixmap {
        QString key = ResourceUtils::findTextureKey(themes, name);
        if (key.isEmpty()) return QPixmap();
        return ResourceUtils::applyMagentaMask(themes[key]);
    };

    // 🔼🔽 + Bar laden
    QPixmap texUp   = getState(loadTex("buttvscrup"),   0);
    QPixmap texDown = getState(loadTex("buttvscrdown"), 0);
    QPixmap texBar  = getState(loadTex("buttvscrbar"),  1);

    if (texBar.isNull()) return;

    // 🔁 Bar spiegeln, überlappen und kombinieren
    QImage base = texBar.toImage();
    QImage mirrored = base.mirrored(true, false);

    int overlap = 2; // schwarze Mitte überlappen
    int totalW = base.width() + mirrored.width() - overlap;
    QImage combined(totalW, base.height(), QImage::Format_ARGB32_Premultiplied);
    combined.fill(Qt::transparent);

    QPainter comb(&combined);
    comb.drawImage(0, 0, base);
    comb.drawImage(base.width() - overlap, 0, mirrored);
    comb.end();

    QPixmap texFull = QPixmap::fromImage(combined);

    const int scrollWidth = texFull.width();
    const int upH   = texUp.isNull()   ? 16 : texUp.height();
    const int downH = texDown.isNull() ? 16 : texDown.height();
    const int barH  = texFull.height();

    QRect scrollRect(rect.right() - scrollWidth + 1, rect.top(), scrollWidth, rect.height());
    QRect upRect(scrollRect.left(), scrollRect.top(), scrollRect.width(), upH);
    QRect downRect(scrollRect.left(),
                   scrollRect.bottom() - downH + 1,
                   scrollRect.width(), downH);
    QRect trackRect(scrollRect.left(),
                    upRect.bottom() + 1,
                    scrollRect.width(),
                    scrollRect.height() - upH - downH - 2);

    // 🟡 Balken rendern
    for (int y = trackRect.top(); y < trackRect.bottom(); y += barH) {
        const int h = std::min(barH, trackRect.bottom() - y);
        QRect target(trackRect.left(), y, scrollWidth, h);
        QRect source(0, 0, scrollWidth, h);
        p.drawPixmap(target, texFull, source);
    }

    // 🔼 / 🔽 Buttons auf volle Breite skalieren, leicht nach außen gezogen
    const int expand = 1; // 1px weiter nach außen
    QRect targetUp(scrollRect.left() - expand,
                   scrollRect.top(),
                   scrollRect.width() + 2 * expand,
                   upH);
    QRect targetDown(scrollRect.left() - expand,
                     scrollRect.bottom() - downH + 1,
                     scrollRect.width() + 2 * expand,
                     downH);

    if (!texUp.isNull()) {
        QPixmap scaledUp = texUp.scaled(targetUp.size(),
                                        Qt::IgnoreAspectRatio,
                                        Qt::SmoothTransformation);
        p.drawPixmap(targetUp, scaledUp);
    }
    if (!texDown.isNull()) {
        QPixmap scaledDown = texDown.scaled(targetDown.size(),
                                            Qt::IgnoreAspectRatio,
                                            Qt::SmoothTransformation);
        p.drawPixmap(targetDown, scaledDown);
    }
}

// =====================================================
//  🔹 Editbox Rendering (überdeckend, keine Lücken)
// =====================================================
void renderEdit(QPainter& p, const QRect& rect,
                const std::shared_ptr<ControlData>& ctrl,
                const QMap<QString, QPixmap>& themes,
                ControlState state)
{
    // 1️⃣ Hintergrund auslagern
    renderEditBackground(p, rect, themes);

    // 2️⃣ (optional) später Text, Cursor, Fokusrahmen o.ä.
    Q_UNUSED(ctrl);
}

// ================================================================
// Text Control (WTYPE_TEXT) – aktuell nur Hintergrund, kein Text
// ================================================================
void renderText(QPainter& p, const QRect& rect,
                const std::shared_ptr<ControlData>& ctrl,
                const QMap<QString, QPixmap>& themes,
                ControlState state)
{
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] Text (placeholder):" << ctrl->id << "Texture:" << ctrl->texture;

    // Hintergrund wie bei Edit – gleiche Tiles, kein Text
    renderEditBackground(p, rect, themes);

    // Später: Textdarstellung
}


// =====================================================
//  🔹 Button-Renderer
// =====================================================

// Standard Buttons
void renderStandardButton(QPainter& p, const QRect& rect,
                          const std::shared_ptr<ControlData>& ctrl,
                          const QMap<QString, QPixmap>& themes,
                          ControlState state)
{
    // 🔹 Transparenz & Blendmodus wie im FlyFF-Client
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] Button:" << ctrl->id << "Texture:" << ctrl->texture;

    // 1️⃣ Direkte Texturverwendung (wenn im Control angegeben)
    if (!ctrl->texture.isEmpty()) {
        QString foundKey = ResourceUtils::findTextureKey(themes, ctrl->texture);
        if (!foundKey.isEmpty()) {
            QPixmap tex = themes.value(foundKey);
            if (!tex.isNull()) {

                // Schneide ggf. Multi-State-Textur in Frames
                TextureStates stages = loadTextureStages(themes, ctrl->texture);

                // Wähle richtigen Frame anhand State
                QPixmap frame;
                switch (state) {
                case ControlState::Hover:    frame = stages.hover;    break;
                case ControlState::Pressed:  frame = stages.pressed;  break;
                case ControlState::Disabled: frame = stages.disabled; break;
                default:                     frame = stages.normal;   break;
                }

                if (frame.isNull()) {
                    frame = tex; // Fallback: ganze Textur
                }

                const int slice = 6; // typischer FlyFF-Rand
                int w = frame.width();
                int h = frame.height();

                // 3-Slice horizontaler Button (linke/mittlere/rechte Zone)
                if (w > slice * 2) {
                    QPixmap left  = frame.copy(0, 0, slice, h);
                    QPixmap mid   = frame.copy(slice, 0, w - slice * 2, h);
                    QPixmap right = frame.copy(w - slice, 0, slice, h);

                    // Zeichnen
                    p.drawPixmap(rect.left(), rect.top(), left);
                    p.drawTiledPixmap(QRect(rect.left() + slice, rect.top(),
                                            rect.width() - slice * 2, rect.height()), mid);
                    p.drawPixmap(rect.right() - slice + 1, rect.top(), right);
                } else {
                    // Einfache Textur – direkt skalieren
                    p.drawPixmap(rect, frame.scaled(rect.size(),
                                                    Qt::IgnoreAspectRatio,
                                                    Qt::SmoothTransformation));
                }

                return;
            }
        }

        qDebug() << "[Render] Keine passende Textur gefunden, versuche Prefix...";
    }

    // 2️⃣ Fallback: Prefix-Suche (z. B. wndbutton / button)
    const QStringList candidates = { "wndbutton", "button" };
    const QString prefix = ResourceUtils::detectTilePrefix(themes, candidates);

    if (prefix.isEmpty()) {
        qDebug() << "[Render] Kein Prefix gefunden für Button – Fallback aktiv!";
        p.fillRect(rect, QColor(60, 60, 60)); // grauer Platzhalter
        return;
    }

    // 3️⃣ Texturen für alle States laden
    TextureStates states = loadTextureStages(themes, prefix + "00");

    QPixmap frame;
    switch (state) {
    case ControlState::Hover:    frame = states.hover;    break;
    case ControlState::Pressed:  frame = states.pressed;  break;
    case ControlState::Disabled: frame = states.disabled; break;
    default:                     frame = states.normal;   break;
    }

    if (!frame.isNull()) {
        p.drawPixmap(rect, frame.scaled(rect.size(),
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
void renderCheckButton(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes,
                       ControlState state)
{
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
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

    // 🔹 Bereichsauswahl: automatische Erkennung der Frames
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

    // 🔹 Zentrierte Darstellung (max. 24×24 px, quadratisch)
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
void renderRadioButton(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes,
                       ControlState state)
{
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
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

    // 🔹 Bereichsauswahl: automatische Erkennung der Frames

    // Fall 1: 8×8-Frames (ältere FlyFF-Themes)
    if (w % 8 == 0 && h == 8 && w / 8 >= 1) {
        int frames = w / 8;
        qDebug() << "[RadioButton Slice Detected]" << frames << "frames of 8x8";
        tex = tex.copy(0, 0, 8, 8);
    }
    // Fall 2: 6 Zustände à 14×14
    else if (w % 6 == 0 && w / 6 == h) {
        int frameWidth = w / 6;
        qDebug() << "[RadioButton 6-State Detected] FrameWidth:" << frameWidth;
        tex = tex.copy(0, 0, frameWidth, h);
    }
    // Fall 3: Klassische 4-State-Textur
    else if (w > h * 4) {
        int frameWidth = w / 4;
        qDebug() << "[RadioButton 4-State Detected] FrameWidth:" << frameWidth;
        tex = tex.copy(0, 0, frameWidth, h);
    }
    else {
        qDebug() << "[RadioButton Single Frame Detected]";
    }

    // 🔹 Zentrierte Darstellung (max. 24×24 px, quadratisch)
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
void renderStatic(QPainter& p, const QRect& rect,
                  const std::shared_ptr<ControlData>& ctrl,
                  const QMap<QString, QPixmap>& themes,
                  ControlState state)
{
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
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
            const int frameCount = 4;
            const int frameWidth = w / frameCount;
            tex = tex.copy(0, 0, frameWidth, h);
        }

        p.drawPixmap(rect, tex.scaled(rect.size(),
                                      Qt::IgnoreAspectRatio,
                                      Qt::SmoothTransformation));
        return;
    }

    // 2️⃣ Kein Bild → einfacher Hintergrund + Rahmen als Platzhalter
    p.setOpacity(1.0);
    p.fillRect(rect, QColor(0, 0, 0, 80));
    p.setPen(QColor(255, 255, 255, 40));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    qDebug() << "[Render] Static without texture – placeholder frame drawn";
}

// ================================================================
// GroupBox
// ================================================================
void renderGroupBox(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes,
                    ControlState state)
{
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
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
}

// ================================================================
// Combobox
// ================================================================
void renderComboBox(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes,
                    ControlState state)
{
    Q_UNUSED(ctrl);

    // Hintergrund exakt wie EditCtrl, aber ohne Text
    renderEditBackground(p, rect, themes);

    // Pfeil rechts
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
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
void renderHorizontalTabCtrl(QPainter& p, const QRect& rect,
                             const std::shared_ptr<ControlData>& ctrl,
                             const QMap<QString, QPixmap>& themes,
                             ControlState state)
{
    Q_UNUSED(ctrl);

    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // 1️⃣ Prefix-Erkennung & Tab-Tiles laden
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

    // 2️⃣ Tab-Höhe bestimmen
    int tabHeight = 20;
    if (!tabActiveM.isNull())
        tabHeight = tabActiveM.height();
    else if (!tabInactiveM.isNull())
        tabHeight = tabInactiveM.height();

    tabHeight = std::clamp(tabHeight, 16, rect.height() / 3);

    const int tabCount = 2;  // Party: Information / Skill
    const int tabWidth = std::max(40, rect.width() / tabCount);

    // 3️⃣ Body zeichnen
    QRect bodyRect = rect.adjusted(0, 0, 0, -tabHeight + 1);

    if (bodyRect.height() > 0) {
        renderEditBackground(p, bodyRect, themes);

        if (!themes.contains("wndedittile00") && !themes.contains("wndtile00"))
            p.fillRect(bodyRect, QColor(20, 20, 20));
    }

    // 4️⃣ Tab-Bar zeichnen
    QRect barRect(rect.left(),
                  bodyRect.bottom() - 1,
                  rect.width(),
                  tabHeight + 1);

    if (!tabInactiveM.isNull())
        p.drawTiledPixmap(barRect, tabInactiveM);
    else
        p.fillRect(barRect, QColor(70, 60, 40)); // Fallback

    // 5️⃣ Tabs zeichnen (links aktiv, rechts inaktiv)
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

    // 6️⃣ Platzhalter-Texte
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

void renderVerticalTabCtrl(QPainter& p, const QRect& rect,
                           const std::shared_ptr<ControlData>& ctrl,
                           const QMap<QString, QPixmap>& themes,
                           ControlState state)
{
    Q_UNUSED(ctrl);

    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    // 🔹 Helper-Funktion zum Laden einer Texture
    auto loadTex = [&](const QString& name) -> QPixmap {
        QString key = ResourceUtils::findTextureKey(themes, name);
        if (key.isEmpty()) return QPixmap();
        return themes.value(key);
    };

    // 🔹 Tab-Tiles unten (WndTabTile10–12, Abschluss = 15)
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

    // 🔹 Geometrie
    QRect contentRect = rect.adjusted(0, 0, 0, -tabHeight);
    contentRect = contentRect.normalized();

    // 🔹 Scrollbar-Breite schätzen (aus Theme)
    int scrollWidth = 16;
    QPixmap texUp = loadTex("buttvscrup");
    if (!texUp.isNull())
        scrollWidth = texUp.width();
    scrollWidth = std::min(scrollWidth, contentRect.width() / 4);

    QRect bodyRect = contentRect.adjusted(0, 0, -scrollWidth, 0);

    // 🔹 Body (linker Inhaltsbereich)
    if (bodyRect.width() > 0) {
        renderEditBackground(p, bodyRect, themes);
        if (!themes.contains("wndedittile00") && !themes.contains("wndtile00"))
            p.fillRect(bodyRect, QColor(15, 15, 15));
    }

    // 🔹 Unterer Tab-Header (mit rechtem Abschluss-Tile 15)
    {
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
                int x = tabRect.right() + 1;
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

    // 🔹 Scrollbar (gemeinsame Routine)
    QRect scrollRect(contentRect.right() - scrollWidth - 2,
                     contentRect.top() + 2,
                     scrollWidth,
                     contentRect.height() - 4);

    renderVerticalScrollBar(p, scrollRect, themes);

    // Lichtkante rechts
    p.setPen(QColor(255, 255, 255, 50));
    p.drawLine(scrollRect.right(), scrollRect.top(), scrollRect.right(), scrollRect.bottom());

    p.restore();
}

// ================================================================
// Tab Control – Automatische Variante (horizontal / vertikal)
// ================================================================
void renderTabCtrl(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes,
                   ControlState state)
{
    bool isVertical = rect.height() > rect.width();
    if (isVertical)
        renderVerticalTabCtrl(p, rect, ctrl, themes, state);
    else
        renderHorizontalTabCtrl(p, rect, ctrl, themes, state);
}

// ================================================================
// ListBox
// ================================================================
void renderListBox(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes,
                   ControlState state)
{
    Q_UNUSED(ctrl);

    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] ListBox (EditBackground):" << ctrl->id << "Texture:" << ctrl->texture;

    // 🔹 Hintergrund wie Edit – gleiche Tiles, keine Verzerrung
    renderEditBackground(p, rect, themes);

    // Optional: dezenter Rahmen oben/unten für mehr „ListBox-Gefühl“
    p.setOpacity(1.0);
    p.setPen(QColor(0, 0, 0, 120));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    p.restore();
}

// =====================================================
//  Tabtree
// =====================================================
void renderTreeCtrl(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes,
                    ControlState state)
{
    constexpr qreal FLYFF_WINDOW_ALPHA_LOCAL = 200.0 / 255.0;
    p.save();
    p.setOpacity(FLYFF_WINDOW_ALPHA_LOCAL);
    p.setCompositionMode(QPainter::CompositionMode_SourceOver);

    qDebug() << "[Render] TreeCtrl:" << ctrl->id << "Texture:" << ctrl->texture;

    // 1️⃣ Hintergrund wie Edit – gleiche Struktur
    renderEditBackground(p, rect, themes);

    // 2️⃣ Scrollbar simulieren (rechte Seite)
    const int scrollWidth = 12;
    QRect scrollRect(rect.right() - scrollWidth, rect.top() + 2, scrollWidth - 2, rect.height() - 4);

    renderVerticalScrollBar(p, scrollRect, themes);

    // 3️⃣ optional Rahmen
    p.setOpacity(1.0);
    p.setPen(QColor(0, 0, 0, 120));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    p.restore();
}

// =====================================================
//  🔹 Haupt-Renderer: renderControl()
// =====================================================
void renderControl(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes,
                   ControlState state = ControlState::Normal)
{
    if (!ctrl) return;

    const QString type   = ctrl->type.toUpper();
    const QString texKey = ctrl->texture.toLower();

    p.save();
    p.fillRect(rect.adjusted(2, 2, -2, -2), QColor(255, 0, 0, 60));
    p.restore();
    qDebug() << "[RenderControl]" << ctrl->id << "Type:" << type << "Texture:" << texKey;

    // 1️⃣ Edit – Speziallogik
    if (type.contains("EDIT")) {
        renderEdit(p, rect, ctrl, themes, state);
        return;
    }

    // 2️⃣ Buttons
    if (type.contains("BUTTON")) {

        // ✅ Check-Button
        if (texKey.contains("buttcheck") || type.contains("CHECK")) {
            renderCheckButton(p, rect, ctrl, themes, state);
            return;
        }

        // ✅ Radio-Button
        if (texKey.contains("buttradio") || type.contains("RADIO")) {
            renderRadioButton(p, rect, ctrl, themes, state);
            return;
        }

        // ✅ Normaler Button
        renderStandardButton(p, rect, ctrl, themes, state);
        return;
    }

    // ComboBox
    if (type.contains("COMBO")) {
        renderComboBox(p, rect, ctrl, themes, state);
        return;
    }

    // Static
    if (type.contains("STATIC")) {
        renderStatic(p, rect, ctrl, themes, state);
        return;
    }

    // Text
    if (type.contains("TEXT")) {
        renderText(p, rect, ctrl, themes, state);
        return;
    }

    // Tab Controls
    if (type.contains("TABCTRL")) {
        if (rect.width() >= rect.height())
            renderHorizontalTabCtrl(p, rect, ctrl, themes, state);
        else
            renderVerticalTabCtrl(p, rect, ctrl, themes, state);
        return;
    }

    // Listbox
    if (type.contains("LISTBOX")) {
        renderListBox(p, rect, ctrl, themes, state);
        return;
    }

    // Groupbox
    if (type.contains("GROUPBOX")) {
        renderGroupBox(p, rect, ctrl, themes, state);
        return;
    }

    // 5️⃣ TreeCtrl
    if (type.contains("TREECTRL")) {
        renderTreeCtrl(p, rect, ctrl, themes, state);
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
