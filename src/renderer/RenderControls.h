#pragma once

#include <QPainter>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <memory>

#include "layout/model/ControlData.h"

// Forward-Decl (kommt aus BehaviorManager.h)
struct BehaviorInfo;

// =====================================================
//  RenderControls – Renderer-Klasse für FlyFF-Controls
// =====================================================
class RenderControls
{
public:
    RenderControls() = default;

    // Haupt-Dispatcher für ein einzelnes Control
    // behavior kann später genutzt werden (Capabilities, States, etc.),
    // aktuell wird es noch nicht ausgewertet.
    void renderControl(QPainter& p,
                       const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes,
                       const BehaviorInfo* behavior = nullptr);


// =====================================================
//  🔹 Hilfsfunktionen
// =====================================================
void drawNineSlice(QPainter& p, const QRect& rect,
                   const QMap<QString, QPixmap>& themes,
                   const QStringList& candidates);

void renderEditBackground(QPainter& p, const QRect& rect,
                          const QMap<QString, QPixmap>& themes);

void renderVerticalScrollBar(QPainter& p, const QRect& rect,
                             const QMap<QString, QPixmap>& themes);

// =====================================================
//  🔹 Renderer für einzelne Controltypen
// =====================================================
void renderEdit(QPainter& p, const QRect& rect,
                const std::shared_ptr<ControlData>& ctrl,
                const QMap<QString, QPixmap>& themes);

void renderText(QPainter& p, const QRect& rect,
                const std::shared_ptr<ControlData>& ctrl,
                const QMap<QString, QPixmap>& themes);

void renderStandardButton(QPainter& p, const QRect& rect,
                          const std::shared_ptr<ControlData>& ctrl,
                          const QMap<QString, QPixmap>& themes);

void renderCheckButton(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes);

void renderRadioButton(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes);

void renderStatic(QPainter& p, const QRect& rect,
                  const std::shared_ptr<ControlData>& ctrl,
                  const QMap<QString, QPixmap>& themes);

void renderGroupBox(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes);

void renderComboBox(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes);

void renderHorizontalTabCtrl(QPainter& p, const QRect& rect,
                             const std::shared_ptr<ControlData>& ctrl,
                             const QMap<QString, QPixmap>& themes);

void renderVerticalTabCtrl(QPainter& p, const QRect& rect,
                           const std::shared_ptr<ControlData>& ctrl,
                           const QMap<QString, QPixmap>& themes);

void renderTabCtrl(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes);

void renderListBox(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes);

void renderTreeCtrl(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes);

};


