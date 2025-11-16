#pragma once
#include <QObject>
#include <QMap>
#include <QString>
#include <QList>
#include <memory>
#include <vector>

#include "utils/BaseManager.h"
#include "layout/model/TokenData.h"

struct WindowData;
struct ControlData;

// ------------------------------------------------------------
// DefineManager
//  - Verwalten und Analysieren aller Define-Daten (#define ...)
//  - Arbeitet mit Token-System zusammen
//  - Erbt von BaseManager (für Dirty-Flag & Änderungs-Signale)
// ------------------------------------------------------------
class DefineManager : public BaseManager
{
    Q_OBJECT
public:
    explicit DefineManager(QObject* parent = nullptr);

    void clear();
    void addDefine(const QString& name, quint32 value);
    void processDefineLine(const QString& line);

    bool hasDefine(const QString& name) const;
    quint32 getValue(const QString& name) const;

    void generateDefines(const QMap<QString, std::shared_ptr<WindowData>>& windows);
    void rebuildFromTokens(const QList<Token>& tokens);

    void importFromTokens(const QList<Token>& tokens);
    QList<Token> exportToTokens() const;

    const QMap<QString, quint32>& allDefines() const { return m_all; }
    const QMap<QString, quint32>& windowDefines() const { return m_windowDefines; }
    const QMap<QString, quint32>& controlDefines() const { return m_controlDefines; }

    void applyDefinesToLayout(const std::vector<std::shared_ptr<WindowData>>& windows);

private:
    QMap<QString, quint32> m_all;
    QMap<QString, quint32> m_windowDefines;
    QMap<QString, quint32> m_controlDefines;
};
