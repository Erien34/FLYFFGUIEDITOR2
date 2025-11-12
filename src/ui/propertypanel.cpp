#include "PropertyPanel.h"
#include "core/ProjectController.h"
#include "behavior/BehaviorManager.h"
#include "layout/model/WindowData.h"
#include "layout/model/ControlData.h"

#include <QLabel>
#include <QScrollArea>
#include <QCheckBox>
#include <QGroupBox>
#include <QSpacerItem>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <qtimer.h>

PropertyPanel::PropertyPanel(ProjectController* controller, QWidget* parent)
    : QWidget(parent), m_controller(controller)
{
    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);

    // ⚠ Kein Parent für container!
    QWidget* container = new QWidget();
    m_layout = new QVBoxLayout(container);
    m_layout->setContentsMargins(6, 6, 6, 6);
    m_layout->setSpacing(8);
    scroll->setWidget(container);


    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(scroll);
    setLayout(mainLayout);
}

// ------------------------------------------------------------
// Fenster-Eigenschaften
// ------------------------------------------------------------
void PropertyPanel::showWindowProps(const std::shared_ptr<WindowData>& wnd)
{
    if (!wnd || !m_controller)
        return;

    QSignalBlocker blocker(this);
    m_isRefreshing = true;

    m_currentControl = nullptr;
    m_currentWindow  = wnd.get();

    clear();

    auto* bm = m_controller->behaviorManager();
    if (!bm)
        return;

    // ==========================================================
    // HEADER-INFOS
    // ==========================================================
    auto* centerContainer = new QWidget(this);
    auto* centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setAlignment(Qt::AlignHCenter);
    centerLayout->setContentsMargins(20, 10, 20, 10);
    centerLayout->setSpacing(4);

    auto addCenteredLabel = [&](const QString& html) {
        QLabel* lbl = new QLabel(html, centerContainer);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        centerLayout->addWidget(lbl);
    };

    addCenteredLabel(QString("<h3>Window: <b>%1</b></h3>").arg(wnd->name));

    addCenteredLabel(QString("<b>Modus:</b> %1").arg(wnd->modus));
    addCenteredLabel(QString("<b>Größe:</b> %1 × %2").arg(wnd->width).arg(wnd->height));
    addCenteredLabel(QString("<b>Mod:</b> %1").arg(wnd->mod));

    if (!wnd->texture.isEmpty())
        addCenteredLabel(QString("<b>Texture:</b> %1").arg(wnd->texture));
    if (!wnd->titletext.isEmpty())
        addCenteredLabel(QString("<b>Title Text:</b> %1").arg(wnd->titletext));
    if (!wnd->titleId.isEmpty())
        addCenteredLabel(QString("<b>Title ID:</b> %1").arg(wnd->titleId));
    if (!wnd->helpId.isEmpty())
        addCenteredLabel(QString("<b>Help ID:</b> %1").arg(wnd->helpId));
    if (!wnd->flagsHex.isEmpty())
        addCenteredLabel(QString("<b>Flags (Hex):</b> %1").arg(wnd->flagsHex));

    if (wnd->isCorrupted)
        addCenteredLabel("<span style='color:red;'><b>WARNUNG:</b> Fenster beschädigt</span>");

    m_layout->addWidget(centerContainer);

    // ==========================================================
    // FLAG-GRUPPE
    // ==========================================================
    const QMap<QString, quint32> allFlags = bm->windowFlags();

    // 🔹 Aktive Flags bestimmen
    QStringList activeFlags = wnd->resolvedMask;

    // 🔹 Regelobjekt laden
    QJsonObject rules;
    const QJsonObject windowRules = bm->windowFlagRules();

    if (!windowRules.isEmpty()) {
        if (windowRules.contains(wnd->name))
            rules = windowRules[wnd->name].toObject();
        else if (windowRules.contains("Default"))
            rules = windowRules["Default"].toObject();
    }

    // ==========================================================
    // FLAG-CHECKBOXEN ERZEUGEN (mit gesetztem Zustand)
    // ==========================================================
    QGroupBox* grp = new QGroupBox("Window Flags", this);
    QVBoxLayout* grpLayout = new QVBoxLayout(grp);

    for (auto it = allFlags.constBegin(); it != allFlags.constEnd(); ++it) {
        const QString flagName = it.key();
        const quint32 flagValue = it.value();

        QCheckBox* cb = new QCheckBox(flagName, grp);

        // ✅ Checked, wenn Bit aktiv oder in resolvedMask
        bool isChecked =
            ((wnd->flagsMask & flagValue) == flagValue) ||
            activeFlags.contains(flagName);

        cb->setChecked(isChecked);

        connect(cb, &QCheckBox::checkStateChanged, this,
                [this, flagValue, wnd](Qt::CheckState state) {
                    quint32 newMask = wnd->flagsMask;
                    if (state == Qt::Checked)
                        newMask |= flagValue;
                    else
                        newMask &= ~flagValue;
                    emit flagsChanged(newMask);
                });

        grpLayout->addWidget(cb);
    }

    grpLayout->addStretch(1);
    grp->setLayout(grpLayout);
    m_layout->addWidget(grp);
    m_layout->addStretch(1);

    m_isRefreshing = false;
}

void PropertyPanel::showControlProps(const std::shared_ptr<WindowData>& wnd,
                                     const std::shared_ptr<ControlData>& ctrl)
{
    if (!ctrl || !m_controller)
        return;

    QSignalBlocker blocker(this);
    m_isRefreshing = true;

    m_currentControl = ctrl.get();
    m_currentWindow = nullptr;

    clear();

    auto* bm = m_controller->behaviorManager();
    if (!bm)
        return;

    // ==========================================================
    // HEADER-INFOS
    // ==========================================================
    auto* centerContainer = new QWidget(this);
    auto* centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setAlignment(Qt::AlignHCenter);
    centerLayout->setContentsMargins(20, 10, 20, 10);
    centerLayout->setSpacing(4);

    auto addCenteredLabel = [&](const QString& html) {
        QLabel* lbl = new QLabel(html, centerContainer);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setTextInteractionFlags(Qt::TextSelectableByMouse);
        centerLayout->addWidget(lbl);
    };

    addCenteredLabel(QString("<h3>Control: <b>%1</b> (%2)</h3>").arg(ctrl->id, ctrl->type));

    if (!ctrl->texture.isEmpty())
        addCenteredLabel(QString("<b>Texture:</b> %1").arg(ctrl->texture));

    addCenteredLabel(QString("<b>Position:</b> (%1, %2) – (%3, %4)")
                         .arg(ctrl->x1).arg(ctrl->y1).arg(ctrl->x2).arg(ctrl->y2));

    if (!ctrl->titleId.isEmpty())
        addCenteredLabel(QString("<b>Title ID:</b> %1").arg(ctrl->titleId));
    if (!ctrl->tooltipId.isEmpty())
        addCenteredLabel(QString("<b>Tooltip ID:</b> %1").arg(ctrl->tooltipId));

    if (ctrl->color.isValid()) {
        QString colorText = QString("<b>Color:</b> RGB(%1, %2, %3)")
        .arg(ctrl->color.red())
            .arg(ctrl->color.green())
            .arg(ctrl->color.blue());
        addCenteredLabel(colorText);
    }

    m_layout->addWidget(centerContainer);

    // ==========================================================
    // FLAG-GRUPPE (mit JSON-Regeln)
    // ==========================================================
    const QMap<QString, quint32> allFlags = bm->controlFlags();
    QStringList activeFlags;

    // ✅ Nur echte Bitmaske berücksichtigen, keine Regel-Defaults
    for (auto it = allFlags.constBegin(); it != allFlags.constEnd(); ++it) {
        const QString& flagName = it.key();
        const quint32 flagValue = it.value();

        if (flagValue != 0 && (ctrl->flagsMask & flagValue) == flagValue)
            activeFlags.append(flagName);
    }

    // 🔹 Regelobjekt für diesen Control-Typ laden
    QJsonObject rules;
    const QJsonObject controlRules = bm->controlFlagRules();

    if (!controlRules.isEmpty()) {
        if (controlRules.contains(ctrl->type))
            rules = controlRules[ctrl->type].toObject();
        else if (controlRules.contains("Default"))
            rules = controlRules["Default"].toObject();
    }

    // 🔹 Flag-Gruppe über createFlagGroup() erstellen (nutzt valid/exclusive)
    QWidget* flagGroup = createFlagGroup(
        "Control Flags",
        allFlags,
        activeFlags,
        rules,
        false // keine Window-Gruppe
        );

    m_layout->addWidget(flagGroup);
    m_layout->addStretch(1);

    m_isRefreshing = false;
}

// ------------------------------------------------------------
// Flag-Groupbox erstellen
// ------------------------------------------------------------
QWidget* PropertyPanel::createFlagGroup(const QString& title,
                                        const QMap<QString, quint32>& allFlags,
                                        const QStringList& activeFlags,
                                        const QJsonObject& rules,
                                        bool isWindowGroup)
{
    auto* container = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(container);

    auto* groupBox = new QGroupBox(title, container);
    auto* layout = new QGridLayout(groupBox);

    // ==========================================================
    // 🔹 Regeln einlesen
    // ==========================================================
    QMap<QString, QStringList> exclusiveRules;
    QStringList validFlags;

    if (rules.contains("exclusive")) {
        QJsonObject excl = rules["exclusive"].toObject();
        for (auto it = excl.begin(); it != excl.end(); ++it) {
            QString key = it.key();
            QStringList list;
            for (const auto& v : it.value().toArray())
                list << v.toString();
            exclusiveRules[key] = list;
        }
    }

    if (rules.contains("valid")) {
        for (const auto& v : rules["valid"].toArray())
            validFlags << v.toString();
    }

    // Farbpalette für exklusive Gruppen (max. 6, wiederholt sich danach)
    QList<QString> colors = {
        "rgba(255, 0, 0, 0.08)",     // rot
        "rgba(0, 255, 0, 0.08)",     // grün
        "rgba(0, 0, 255, 0.08)",     // blau
        "rgba(255, 255, 0, 0.08)",   // gelb
        "rgba(255, 0, 255, 0.08)",   // magenta
        "rgba(0, 255, 255, 0.08)"    // cyan
    };

    QMap<QString, QString> exclGroupColor;  // FlagName → Farbe
    int colorIndex = 0;

    for (auto it = exclusiveRules.begin(); it != exclusiveRules.end(); ++it) {
        QString color = colors[colorIndex++ % colors.size()];
        for (const auto& f : it.value())
            exclGroupColor[f] = color;
        exclGroupColor[it.key()] = color;
    }

    // ==========================================================
    // 🔹 Checkboxen erzeugen
    // ==========================================================
    int row = 0, col = 0;
    const int maxCols = 2;

    for (auto it = allFlags.constBegin(); it != allFlags.constEnd(); ++it) {
        const QString flagName = it.key();
        const quint32 flagMask = it.value();

        auto* cb = new QCheckBox(flagName, groupBox);

        // ✅ Nur aus echter Mask (nicht aus Regeln)
        cb->setChecked(activeFlags.contains(flagName));

        // 🟥 Exklusive Gruppen farblich markieren
        if (exclGroupColor.contains(flagName))
            cb->setStyleSheet(QString("background-color: %1;").arg(exclGroupColor.value(flagName)));

        // 🟪 Ungültige Flags ausgrauen
        if (!validFlags.isEmpty() && !validFlags.contains(flagName)) {
            cb->setEnabled(false);
            cb->setToolTip("Dieses Flag ist für diesen Control-Typ nicht gültig.");
            cb->setStyleSheet("color: gray;");
        }

        layout->addWidget(cb, row, col);
        if (++col >= maxCols) { col = 0; ++row; }

        // Exklusivverhalten selbst beibehalten
        if (exclusiveRules.contains(flagName)) {
            connect(cb, &QCheckBox::toggled, this,
                    [this, layout, flagName, exclusiveRules](bool checked) {
                        QStringList excl = exclusiveRules.value(flagName);
                        if (checked) {
                            for (auto* other : layout->findChildren<QCheckBox*>()) {
                                if (excl.contains(other->text()))
                                    other->setChecked(false);
                            }
                        }
                    });
        }

        connect(cb, &::QCheckBox::checkStateChanged, this,
                [this, flagMask, flagName, isWindowGroup](int state) {
                    if (m_isRefreshing)
                        return;
                    const bool checked = (state == Qt::Checked);
                    if (isWindowGroup && m_currentWindow)
                        m_controller->updateWindowFlags(m_currentWindow->name, flagMask, checked);
                    else if (m_currentControl)
                        m_controller->updateControlFlags(m_currentControl->id, flagMask, checked);
                });
    }

    groupBox->setLayout(layout);
    mainLayout->addWidget(groupBox);

    // ==========================================================
    // 🔹 Farblegende unten hinzufügen
    // ==========================================================
    if (!exclusiveRules.isEmpty()) {
        auto* legend = new QWidget(container);
        auto* grid = new QGridLayout(legend);
        grid->setHorizontalSpacing(6);
        grid->setVerticalSpacing(4);
        grid->setContentsMargins(4, 4, 4, 4);

        const int maxCols = 3; // 🔹 Max. 3 Elemente pro Zeile
        int row = 0, col = 0;
        int groupIndex = 1;

        for (auto it = exclusiveRules.begin(); it != exclusiveRules.end(); ++it) {
            QString color = exclGroupColor[it.key()];
            auto* lbl = new QLabel(QString("Exklusiv-Gruppe %1").arg(groupIndex++), legend);
            lbl->setAlignment(Qt::AlignCenter);
            lbl->setFixedHeight(22);
            lbl->setStyleSheet(QString(
                                   "background-color: %1;"
                                   "border: 1px solid #777;"
                                   "border-radius: 4px;"
                                   "padding: 2px 6px;"
                                   ).arg(color));

            grid->addWidget(lbl, row, col, Qt::AlignCenter);

            if (++col >= maxCols) {
                col = 0;
                ++row;
            }
        }

        legend->setLayout(grid);
        mainLayout->addWidget(legend);
    }

    container->setLayout(mainLayout);
    return container;
}


// ------------------------------------------------------------
void PropertyPanel::clear()
{
    if (!m_layout)
        return;

    qInfo() << "[PropertyPanel] clear() – lösche alte UI-Elemente sanft.";

    // 🔸 Alle Signalverbindungen von alten Widgets lösen (bevor delete)
    QList<QWidget*> children = findChildren<QWidget*>();
    for (QWidget* w : children)
        w->disconnect(this); // trennt nur Verbindungen zu PropertyPanel

    // 🔸 Sicheres Entfernen der Widgets
    QSignalBlocker blocker(this);
    setUpdatesEnabled(false);

    // Lösche Widgets synchron (nicht verzögert)
    while (QLayoutItem* item = m_layout->takeAt(0)) {
        QWidget* child = item->widget();
        if (child) {
            child->setParent(nullptr);
            child->deleteLater();  // 💡 asynchron löschen — kein Crash!
        }
        delete item;
    }

    // 🔸 Layout stabilisieren
    m_layout->addStretch(1);
    setUpdatesEnabled(true);
    m_layout->invalidate();
    updateGeometry();
    update();

    qInfo() << "[PropertyPanel] clear() abgeschlossen.";
}

void PropertyPanel::refreshAfterLayoutLoad()
{
    qInfo() << "[PropertyPanel] LayoutsReady empfangen – aktualisiere Anzeige.";
    if (!m_controller || !m_controller->layoutManager())
        return;

    // Falls gerade ein Fenster oder Control selektiert ist → neu anzeigen
    const auto currentWnd = m_controller->currentWindow();
    const auto currentCtrl = m_controller->currentControl();

    if (currentCtrl)
        showControlProps(currentWnd, currentCtrl);
    else if (currentWnd)
        showWindowProps(currentWnd);
}
