#pragma once
#include <QObject>
#include <QMap>
#include <QList>
#include <QString>
#include <vector>
#include <memory>

#include "layout/model/TokenData.h"
#include "utils/BaseManager.h"

struct WindowData;
struct ControlData;

// ------------------------------------------------------------
// Datenstruktur für Textgruppen
// ------------------------------------------------------------
struct TextGroup {
    QString tid;               // Gruppenname (z. B. TID_APP_MAIN)
    QList<QString> ids;        // Zugehörige Text-IDs
};

// ------------------------------------------------------------
// TextManager – zentrale Verwaltung aller Textdaten
// ------------------------------------------------------------
class TextManager : public BaseManager
{
    Q_OBJECT
public:
    explicit TextManager(QObject* parent = nullptr)
        : BaseManager(parent)
    {
        qInfo() << "[TextManager] Initialisiert";
    }

    // ------------------------------------------------------------
    // Grundoperationen
    // ------------------------------------------------------------
    void clear();          // löscht alles
    void clearIncState();  // löscht nur TID-/IDS-Gruppen

    // ------------------------------------------------------------
    // Verarbeitung von Zeilen aus Backend
    // ------------------------------------------------------------
    void processTextLine(const QString& line);  // textClient.txt
    void processIncLine(const QString& line);   // textClient.inc

    // ------------------------------------------------------------
    // Zugriffsfunktionen
    // ------------------------------------------------------------
    QString value(const QString& id) const;
    QString groupForId(const QString& id) const;
    QList<QString> idsForGroup(const QString& tid) const;
    QStringList allGroups() const;
    QMap<QString, QString> allTexts() const { return m_texts; }

    // ------------------------------------------------------------
    // Aufbauhilfen (intern oder für Backend)
    // ------------------------------------------------------------
    void addGroup(const QString& tid);
    void addIdToGroup(const QString& tid, const QString& id);

    // ------------------------------------------------------------
    // Token-Integration
    // ------------------------------------------------------------
    void rebuildFromTokens(const QList<Token>& tokens);

    void applyTextsToLayout(const std::vector<std::shared_ptr<WindowData>>& windows);

private:
    // IDS → Text
    QMap<QString, QString> m_texts;

    // TID → Gruppe
    QMap<QString, TextGroup> m_groups;

    // IDS → TID (schneller Lookup)
    QMap<QString, QString> m_idToGroup;

    QString m_currentTid;
};
