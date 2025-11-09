#pragma once

#include <QPainter>
#include <QPixmap>
#include <QMap>
#include <QString>
#include <memory>

#include "layout/model/ControlData.h"

// =====================================================
//  RenderControls – Hauptmodul für FlyFF UI Rendering
// =====================================================

namespace RenderControls
{

// =====================================================
//  🔹 Allgemeine Statestruktur für Controls
// =====================================================

struct ControlStates
{
    QPixmap normal;
    QPixmap hover;
    QPixmap pressed;
    QPixmap disabled;

    bool isValid() const { return !normal.isNull(); }
};

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

ControlStates loadControlStates(const QMap<QString, QPixmap>& themes,
                                const QString& baseKey);

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

// =====================================================
//  🔹 Haupt-Dispatcher
// =====================================================
void renderControl(QPainter& p, const QRect& rect,
                   const std::shared_ptr<ControlData>& ctrl,
                   const QMap<QString, QPixmap>& themes);

} // namespace RenderControls
