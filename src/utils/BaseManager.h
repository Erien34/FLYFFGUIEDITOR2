#pragma once
#include <QObject>
#include <QDebug>

// ------------------------------------------------------------
// BaseManager – gemeinsame Grundlogik für alle Manager
// ------------------------------------------------------------
//  - Stellt einheitliches Dirty-Flag bereit
//  - Sendet Signal bei Änderungen
//  - Wird von Layout-, Text- und DefineManager geerbt
// ------------------------------------------------------------
class BaseManager : public QObject
{
    Q_OBJECT
public:
    explicit BaseManager(QObject* parent = nullptr)
        : QObject(parent)
    {}

    // Gibt an, ob der Manager geänderte Daten hat
    bool isDirty() const { return m_dirty; }

    // Setzt oder löscht den Dirty-Zustand
    void setDirty(bool dirty = true)
    {
        if (m_dirty == dirty)
            return;
        m_dirty = dirty;

        if (m_dirty)
            emit changed();
    }

    // Rücksetzen des Dirty-Zustands
    void clearDirty() { m_dirty = false; }

signals:
    void changed(); // wird gesendet, wenn Daten geändert wurden

protected:
    bool m_dirty = false;
};
