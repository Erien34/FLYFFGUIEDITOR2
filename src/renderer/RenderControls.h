#pragma once

#include <QPainter>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <memory>

#include "layout/model/ControlData.h"
#include "renderer/ControlState.h"


struct TextureStates
{
    QPixmap normal;
    QPixmap hover;
    QPixmap pressed;
    QPixmap disabled;
};

// Alle Deklarationen im Namespace – passend zu deiner .cpp
namespace RenderControls {

// Haupt-Dispatcher für ein einzelnes Control
void renderControl(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes,
                   ControlState state);

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
                const QMap<QString, QPixmap>& themes,
                ControlState state = ControlState::Normal);

void renderText(QPainter& p, const QRect& rect,
                const std::shared_ptr<ControlData>& ctrl,
                const QMap<QString, QPixmap>& themes,
                ControlState state = ControlState::Normal);

void renderStandardButton(QPainter& p, const QRect& rect,
                          const std::shared_ptr<ControlData>& ctrl,
                          const QMap<QString, QPixmap>& themes,
                          ControlState state = ControlState::Normal);

void renderCheckButton(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes,
                       ControlState state = ControlState::Normal);

void renderRadioButton(QPainter& p, const QRect& rect,
                       const std::shared_ptr<ControlData>& ctrl,
                       const QMap<QString, QPixmap>& themes,
                       ControlState state = ControlState::Normal);

void renderStatic(QPainter& p, const QRect& rect,
                  const std::shared_ptr<ControlData>& ctrl,
                  const QMap<QString, QPixmap>& themes,
                  ControlState state = ControlState::Normal);

void renderGroupBox(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes,
                    ControlState state = ControlState::Normal);

void renderComboBox(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes,
                    ControlState state = ControlState::Normal);

void renderHorizontalTabCtrl(QPainter& p, const QRect& rect,
                             const std::shared_ptr<ControlData>& ctrl,
                             const QMap<QString, QPixmap>& themes,
                             ControlState state = ControlState::Normal);

void renderVerticalTabCtrl(QPainter& p, const QRect& rect,
                           const std::shared_ptr<ControlData>& ctrl,
                           const QMap<QString, QPixmap>& themes,
                           ControlState state = ControlState::Normal);

void renderTabCtrl(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes,
                   ControlState state = ControlState::Normal);

void renderListBox(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes,
                   ControlState state = ControlState::Normal);

void renderTreeCtrl(QPainter& p, const QRect& rect,
                    const std::shared_ptr<ControlData>& ctrl,
                    const QMap<QString, QPixmap>& themes,
                    ControlState state = ControlState::Normal);

} // namespace RenderControls
