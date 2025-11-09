#include "LayoutBackend.h"
#include "FileManager.h"

#include <QFile>
#include <QJsonDocument>
#include <QTextStream>
#include <QDebug>

// =============================================================
// Konstruktoren
// =============================================================
LayoutBackend::LayoutBackend(FileManager& fm, LayoutParser& parser)
    : m_fileManager(&fm), m_parser(parser)
{
}

LayoutBackend::LayoutBackend(LayoutParser& parser)
    : m_parser(parser)
{
}

// =============================================================
// Öffentliche API (parameterlos) – Aufrufe vom LayoutManager
// =============================================================

QJsonObject LayoutBackend::loadWindowFlags()
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für loadWindowFlags()";
        return {};
    }
    return loadWindowFlags(m_fileManager->windowFlagsPath());
}

QJsonObject LayoutBackend::loadControlFlags()
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für loadControlFlags()";
        return {};
    }
    return loadControlFlags(m_fileManager->controlFlagsPath());
}

QJsonObject LayoutBackend::loadWindowTypes()
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für loadWindowTypes()";
        return {};
    }
    return loadWindowTypes(m_fileManager->windowTypesPath());
}

bool LayoutBackend::saveWindowFlagRules(const QJsonObject& json)
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für saveWindowFlagRules()";
        return false;
    }
    return saveWindowFlagRules(m_fileManager->windowFlagRulesPath(), json);
}

QJsonObject LayoutBackend::loadWindowFlagRules()
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für loadWindowFlagRules()";
        return {};
    }
    return loadWindowFlagRules(m_fileManager->windowFlagRulesPath());
}

QJsonObject LayoutBackend::loadControlFlagRules()
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für loadControlFlagRules()";
        return {};
    }
    return loadControlFlagRules(m_fileManager->controlFlagRulesPath());
}


bool LayoutBackend::saveControlFlagRules(const QJsonObject& json)
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für saveControlFlagRules()";
        return false;
    }
    return saveControlFlagRules(m_fileManager->controlFlagRulesPath(), json);
}

QJsonObject LayoutBackend::loadUndefinedControlFlags()
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für loadUndefinedControlFlags()";
        return {};
    }
    return loadUndefinedControlFlags(m_fileManager->undefinedControlFlagsPath());
}

bool LayoutBackend::saveUndefinedControlFlags(const QJsonObject& json)
{
    if (!m_fileManager)
    {
        qWarning() << "[LayoutBackend] Kein FileManager gesetzt für saveUndefinedControlFlags()";
        return false;
    }
    return saveUndefinedControlFlags(m_fileManager->undefinedControlFlagsPath(), json);
}

// =============================================================
// Private Implementierungen mit Pfadangabe
// =============================================================

QJsonObject LayoutBackend::loadWindowFlags(const QString& path)
{
    return m_fileManager ? m_fileManager->loadJsonObject(path) : QJsonObject{};
}

QJsonObject LayoutBackend::loadControlFlags(const QString& path)
{
    return m_fileManager ? m_fileManager->loadJsonObject(path) : QJsonObject{};
}

QJsonObject LayoutBackend::loadWindowTypes(const QString& path)
{
    return m_fileManager ? m_fileManager->loadJsonObject(path) : QJsonObject{};
}

QJsonObject LayoutBackend::loadWindowFlagRules(const QString& path)
{
    return m_fileManager ? m_fileManager->loadJsonObject(path) : QJsonObject{};
}

QJsonObject LayoutBackend::loadControlFlagRules(const QString& path)
{
    return m_fileManager ? m_fileManager->loadJsonObject(path) : QJsonObject{};
}

QJsonObject LayoutBackend::loadUndefinedControlFlags(const QString& path)
{
    return m_fileManager ? m_fileManager->loadJsonObject(path) : QJsonObject{};
}

bool LayoutBackend::saveWindowFlagRules(const QString& path, const QJsonObject& json)
{
    return m_fileManager ? m_fileManager->saveJsonObject(path, json) : false;
}

bool LayoutBackend::saveControlFlagRules(const QString& path, const QJsonObject& json)
{
    return m_fileManager ? m_fileManager->saveJsonObject(path, json) : false;
}

bool LayoutBackend::saveUndefinedControlFlags(const QString& path, const QJsonObject& json)
{
    return m_fileManager ? m_fileManager->saveJsonObject(path, json) : false;
}

// =============================================================
// Allgemeine Lade-/Speicherfunktionen
// =============================================================
bool LayoutBackend::load()
{
    if (m_path.isEmpty()) {
        qWarning() << "[LayoutBackend] Kein Layout-Pfad gesetzt – setPath() vorher aufrufen!";
        return false;
    }

    QFileInfo fi(m_path);
    if (!fi.exists()) {
        qWarning() << "[LayoutBackend] Layout-Datei existiert nicht:" << m_path;
        return false;
    }

    qInfo() << "[LayoutBackend] Lade Layout-Datei:" << m_path;

    // 🔹 Hier startet wirklich das Parsen
    bool ok = m_parser.parse(m_path);

    if (!ok) {
        qWarning() << "[LayoutBackend] LayoutParser konnte Datei nicht verarbeiten:" << m_path;
        return false;
    }

    qInfo() << "[LayoutBackend] Layout-Datei erfolgreich geladen und Tokens erzeugt.";
    return true;
}

bool LayoutBackend::save()
{
    // Optionaler Sammelspeichervorgang
    return true;
}

// =============================================================
// Hilfsfunktionen
// =============================================================
void LayoutBackend::setPath(const QString& p)
{
    m_path = p;
}

bool LayoutBackend::writeFile(const QString& path, const QString& content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << content;
    return true;
}
