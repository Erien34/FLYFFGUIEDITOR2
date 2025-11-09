#pragma once
#include <QString>
#include <QJsonObject>
#include "LayoutParser.h"

class FileManager;

// ------------------------------------------------------------
// LayoutBackend
// ------------------------------------------------------------
// Verantwortlich für Datei-I/O des Layoutsystems.
// Kennt die Pfade über den FileManager, kümmert sich um
// Lesen und Schreiben von JSON-Dateien für Flags, Typen usw.
// ------------------------------------------------------------
class LayoutBackend
{
public:
    explicit LayoutBackend(FileManager& fm, LayoutParser& parser);
    explicit LayoutBackend(LayoutParser& parser); // Fallback ohne FileManager

    // ------------------------------------------------------------
    // Öffentliche API (parameterlos)
    // ------------------------------------------------------------
    bool load();  // optionaler Sammelladevorgang
    bool save();  // optionaler Sammelspeichervorgang

    // Flags / Typen laden
    QJsonObject loadWindowFlags();
    QJsonObject loadControlFlags();
    QJsonObject loadWindowTypes();

    // Regeldateien speichern (Manager liefert JSON)
    bool saveWindowFlagRules(const QJsonObject& json);
    bool saveControlFlagRules(const QJsonObject& json);

    // Unbekannte / undefinierte Control-Flags
    QJsonObject loadUndefinedControlFlags();
    bool saveUndefinedControlFlags(const QJsonObject& json);

    // ------------------------------------------------------------
    // Allgemeine Dateiverwaltung
    // ------------------------------------------------------------
    void setPath(const QString& p);
    bool writeFile(const QString& path, const QString& content);

private:
    // ------------------------------------------------------------
    // Interne Implementierungen mit Pfadangabe
    // ------------------------------------------------------------
    QJsonObject loadWindowFlags(const QString& path);
    QJsonObject loadControlFlags(const QString& path);
    QJsonObject loadWindowTypes(const QString& path);

    QJsonObject loadWindowFlagRules(const QString& path);
    QJsonObject loadControlFlagRules(const QString& path);

    QJsonObject loadUndefinedControlFlags(const QString& path);
    bool saveUndefinedControlFlags(const QString& path, const QJsonObject& json);

    bool saveWindowFlagRules(const QString& path, const QJsonObject& json);
    bool saveControlFlagRules(const QString& path, const QJsonObject& json);

private:
    FileManager* m_fileManager = nullptr;
    LayoutParser& m_parser;
    QString m_path;
};
