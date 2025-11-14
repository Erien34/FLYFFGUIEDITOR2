#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "LayoutParser.h"
#include "WindowData.h"
#include "ControlData.h"
#include "BehaviorManager.h"

class LayoutBackend;
class BehaviorManager;

// ================================================================
// LayoutManager – kümmert sich um Layoutstruktur & Serialisierung
// ================================================================
class LayoutManager : public QObject
{
    Q_OBJECT

public:
    explicit LayoutManager(LayoutParser& parser, LayoutBackend& backend);

    // Behavior anbinden
    void setBehaviorManager(BehaviorManager* behavior) { m_behaviorManager = behavior; }

    // ------------------------------
    // 🔹 Datenaktualisierung
    // ------------------------------
    void refreshFromParser();

    // ------------------------------
    // 🔹 Layout-Verarbeitung
    //    (ruft BehaviorManager für Validierung/Analyse auf)
    // ------------------------------
    void processLayout();

    // ------------------------------
    // 🔹 Serialisierung / Suche
    // ------------------------------
    QString serializeLayout() const;
    std::shared_ptr<WindowData> findWindow(const QString& name) const;

    // ------------------------------
    // 🔹 Zugriff
    // ------------------------------
    const std::vector<std::shared_ptr<WindowData>>& processedWindows() const
    {
        return m_windows;
    }

    // Für BehaviorManager: Zugriff auf Backend
    LayoutBackend& backend()             { return m_backend; }
    const LayoutBackend& backend() const { return m_backend; }

signals:
    void tokensReady();

private:
    LayoutParser&   m_parser;
    LayoutBackend&  m_backend;
    BehaviorManager* m_behaviorManager;

    std::vector<std::shared_ptr<WindowData>> m_windows;

    QString unquote(const QString& s) const;
};
