#pragma once

#include <QObject>
#include <QMap>
#include <QSet>
#include <QJsonObject>
#include <QString>
#include <memory>
#include <vector>

#include "LayoutParser.h"
#include "model/WindowData.h"
#include "model/ControlData.h"


class LayoutBackend;

// ================================================================
// LayoutManager – verarbeitet, validiert und generiert Layoutdaten
// ================================================================
class LayoutManager : public QObject
{
    Q_OBJECT

public:
    explicit LayoutManager(LayoutParser& parser, LayoutBackend& backend);

    // ------------------------------
    // 🔹 Datenaktualisierung
    // ------------------------------
    void refreshFromFiles(const QString& wndPath, const QString& ctrlPath);
    void refreshFromParser();
    void rebuildFromTokens(const QMap<QString, QList<Token>>& tokens);

    // ------------------------------
    // 🔹 Layout-Verarbeitung
    // ------------------------------
    void processLayout();

    // ------------------------------
    // 🔹 Flag-Validierung
    // ------------------------------
    void validateWindowFlags(WindowData* win);
    void validateControlFlags(ControlData* ctrl);

    // ------------------------------
    // 🔹 Analyse und Filterung
    // ------------------------------
    void analyzeControlTypes();
    bool allowsPropertyForType(const QString& type, const QString& property) const;

    // ------------------------------
    // 🔹 Regeln & Flag-Gruppen
    // ------------------------------
    void loadFlagRules();
    QJsonObject getWindowFlagRules() const;
    QJsonObject getControlFlagRules() const;

    QSet<QString> allowedControlFlags(const QString& typeName) const;
    QSet<QString> allowedWindowFlags(const QString& typeName) const;

    void generateDefaultWindowFlagRules();
    void generateDefaultControlFlagRules();

    // ------------------------------
    // 🔹 Unknown Controls
    // ------------------------------
    void generateUnknownControls();  // NEU: ergänzt undefinedControlBits in control_flags.json

    // ------------------------------
    // 🔹 Flags aktualisieren
    // ------------------------------
    void updateControlFlags(const std::shared_ptr<ControlData>& ctrl);
    void updateWindowFlags(const std::shared_ptr<WindowData>& wnd);

    // ------------------------------
    // 🔹 Serialisierung / Suche
    // ------------------------------
    QString serializeLayout() const;
    std::shared_ptr<WindowData> findWindow(const QString& name) const;

    // ------------------------------
    // 🔹 Zugriff
    // ------------------------------
    const std::vector<std::shared_ptr<WindowData>>& processedWindows() const { return m_windows; }
    const QMap<QString, quint32>& windowFlags() const { return m_windowFlags; }
    const QMap<QString, quint32>& controlFlags() const { return m_controlFlags; }
    const QMap<QString, quint32>& windowTypes() const { return m_windowTypes; }

signals:
    void tokensReady();

private:
    // ------------------------------
    // 🔧 Member
    // ------------------------------
    LayoutParser& m_parser;
    LayoutBackend& m_backend;

    std::vector<std::shared_ptr<WindowData>> m_windows;
    QMap<QString, quint32> m_windowFlags;
    QMap<QString, quint32> m_controlFlags;
    QMap<QString, quint32> m_windowTypes;

    // Regeln (aus window_flag_rules.json / control_flag_rules.json)
    mutable QJsonObject m_windowRules;
    mutable QJsonObject m_controlRules;
    QMap<QString, QSet<QString>> m_validFlagsByType;

    // Analyse
    QMap<QString, QSet<QString>> m_controlTypeProperties;

    // 🆕 Gesammelte unbekannte Bits pro Control-Typ
    QMap<QString, QMap<QString, QString>> m_unknownControlBits;

    // ------------------------------
    // 🔹 Hilfsfunktionen
    // ------------------------------
    QString unquote(const QString& s) const;
    void normalizeWindowFlags();   // (optional: deprecated, falls über Backend erledigt)
    void normalizeControlFlags();  // (optional: deprecated)
};
