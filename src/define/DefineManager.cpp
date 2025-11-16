#include "DefineManager.h"
#include "TokenData.h"
#include "WindowData.h"
#include "ControlData.h"

#include <QRegularExpression>
#include <QDebug>

DefineManager::DefineManager(QObject* parent)
    : BaseManager(parent)
{
    qInfo() << "[DefineManager] Initialisiert.";
}

// --------------------------------------------------
// Alles löschen
// --------------------------------------------------
void DefineManager::clear()
{
    m_all.clear();
    m_windowDefines.clear();
    m_controlDefines.clear();
    setDirty();
}

// --------------------------------------------------
// Neues Define hinzufügen oder aktualisieren
// --------------------------------------------------
void DefineManager::addDefine(const QString& name, quint32 value)
{
    if (m_all.value(name) == value)
        return; // keine Änderung

    m_all[name] = value;

    if (name.startsWith("APP_") || name.startsWith("WND_", Qt::CaseInsensitive))
        m_windowDefines[name] = value;
    else if (name.startsWith("WIDC_") || name.startsWith("WTYPE_", Qt::CaseInsensitive))
        m_controlDefines[name] = value;

    setDirty();
}

// --------------------------------------------------
// Eine Zeile aus einer Define-Datei verarbeiten
// --------------------------------------------------
void DefineManager::processDefineLine(const QString& line)
{
    QString trimmed = line.trimmed();
    if (!trimmed.startsWith("#define"))
        return;

    const QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() < 3)
        return;

    QString name = parts[1];
    bool ok = false;
    quint32 value = parts[2].toUInt(&ok, 0);
    if (ok)
        addDefine(name, value);
}

// --------------------------------------------------
// Prüfen, ob ein Define existiert
// --------------------------------------------------
bool DefineManager::hasDefine(const QString& name) const
{
    return m_all.contains(name);
}

// --------------------------------------------------
// Wert eines Define abrufen
// --------------------------------------------------
quint32 DefineManager::getValue(const QString& name) const
{
    return m_all.value(name, 0);
}

// --------------------------------------------------
// Defines generieren (z. B. aus Fensterdaten)
// --------------------------------------------------
void DefineManager::generateDefines(const QMap<QString, std::shared_ptr<WindowData>>& windows)
{
    m_windowDefines.clear();
    m_controlDefines.clear();

    quint32 nextWindowId = 100;
    quint32 baseStep = 1000;

    for (auto it = windows.constBegin(); it != windows.constEnd(); ++it)
    {
        QString wndName = it.key();
        auto wnd = it.value();

        QString wndDefine = QString("WND_%1").arg(wndName.toUpper());
        addDefine(wndDefine, nextWindowId);

        quint32 nextControlId = nextWindowId * baseStep;

        for (const auto& ctrl : wnd->controls)
        {
            QString ctrlName = ctrl->id;
            QString ctrlDefine = QString("WIDC_%1_%2")
                                     .arg(wndName.toUpper())
                                     .arg(ctrlName.toUpper());
            addDefine(ctrlDefine, nextControlId++);
        }

        nextWindowId++;
    }

    qInfo().noquote() << QString("[DefineManager] GenerateDefines abgeschlossen: %1 Fenster, %2 Controls")
                             .arg(m_windowDefines.size())
                             .arg(m_controlDefines.size());

    setDirty();
}

// --------------------------------------------------
// Rebuild aller Defines aus Tokens
// --------------------------------------------------
void DefineManager::rebuildFromTokens(const QList<Token>& tokens)
{
    clear();

    for (const Token& t : tokens)
    {
        if (t.type == "Define")
        {
            QString line = t.value.trimmed();
            if (!line.startsWith("#define"))
                continue;

            const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() < 3)
                continue;

            QString name = parts[1];
            bool ok = false;
            quint32 value = parts[2].toUInt(&ok, 0);
            if (ok)
                addDefine(name, value);
        }
    }

    qInfo() << "[DefineManager] rebuildFromTokens() abgeschlossen:"
            << m_windowDefines.size() << "Fenster,"
            << m_controlDefines.size() << "Controls.";

    setDirty();
}

// --------------------------------------------------
// Import (z. B. von externen Tokens)
// --------------------------------------------------
void DefineManager::importFromTokens(const QList<Token>& tokens)
{
    rebuildFromTokens(tokens);
    setDirty();
}

// --------------------------------------------------
// Export zurück in Tokens
// --------------------------------------------------
QList<Token> DefineManager::exportToTokens() const
{
    QList<Token> tokens;

    for (auto it = m_all.constBegin(); it != m_all.constEnd(); ++it)
    {
        Token t;
        t.type = "Define";
        t.value = QString("#define %1 0x%2")
                      .arg(it.key())
                      .arg(QString::number(it.value(), 16).toUpper());
        tokens.append(t);
    }

    return tokens;
}

void DefineManager::applyDefinesToLayout(const std::vector<std::shared_ptr<WindowData>>& windows)
{
    if (windows.empty()) {
        qInfo() << "[DefineManager] applyDefinesToLayout(): keine Fenster übergeben.";
        return;
    }

    if (m_all.isEmpty() && m_windowDefines.isEmpty() && m_controlDefines.isEmpty()) {
        qInfo() << "[DefineManager] applyDefinesToLayout(): keine Defines geladen – überspringe.";
        return;
    }

    qInfo() << "[DefineManager] applyDefinesToLayout(): Mapping startet. Fenster:" << windows.size();

    for (const auto& wnd : windows)
    {
        if (!wnd)
            continue;

        const QString wndName = wnd->name;
        if (wndName.isEmpty())
            continue;

        // Fenster-Define-Name wie in generateDefines()
        const QString wndDefine = QStringLiteral("WND_%1").arg(wndName.toUpper());

        quint32 wndId = 0;

        if (m_windowDefines.contains(wndDefine))
            wndId = m_windowDefines.value(wndDefine);
        else if (m_all.contains(wndDefine))
            wndId = m_all.value(wndDefine);

        if (wndId != 0) {
            wnd->behavior.attributes["defineName"] = wndDefine;
            wnd->behavior.attributes["defineId"]   = wndId;
        }

        // Controls
        for (const auto& ctrl : wnd->controls)
        {
            if (!ctrl)
                continue;

            const QString ctrlName = ctrl->id;
            if (ctrlName.isEmpty())
                continue;

            // Control-Define-Name wie in generateDefines()
            const QString ctrlDefine = QStringLiteral("WIDC_%1_%2")
                                           .arg(wndName.toUpper())
                                           .arg(ctrlName.toUpper());

            quint32 ctrlId = 0;

            if (m_controlDefines.contains(ctrlDefine))
                ctrlId = m_controlDefines.value(ctrlDefine);
            else if (m_all.contains(ctrlDefine))
                ctrlId = m_all.value(ctrlDefine);

            if (ctrlId != 0) {
                ctrl->behavior.attributes["defineName"] = ctrlDefine;
                ctrl->behavior.attributes["defineId"]   = ctrlId;
            }
        }
    }

    qInfo() << "[DefineManager] applyDefinesToLayout(): Mapping abgeschlossen.";
}
