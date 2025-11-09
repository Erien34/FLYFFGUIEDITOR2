#include "core/FileManager.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QDebug>
#include <qjsondocument.h>
#include <qjsonobject.h>

namespace {
// Hilfsfunktion: vergleicht Dateinamen ohne Rücksicht auf Groß-/Kleinschreibung
bool nameEqualsIgnoreCase(const QString& fileName, const QString& target) {
    return fileName.compare(target, Qt::CaseInsensitive) == 0;
}
}

// ------------------------------------------------------
// Suche NUR nach textclient.inc (bzw. ähnliche Dateien)
// ------------------------------------------------------
QString FileManager::findTextIncFile(const QString& layoutFile) const
{
    QFileInfo layoutInfo(layoutFile);
    if (!layoutInfo.exists() || !layoutInfo.isFile()) {
        qWarning() << "[FileManager] Ungültiger Layout-Pfad:" << layoutFile;
        return {};
    }

    const QString baseDir = layoutInfo.absolutePath();
    const QStringList candidates = { "textclient.inc", "TextClient.inc" };

    QDirIterator it(baseDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        for (const auto& name : candidates) {
            if (nameEqualsIgnoreCase(fi.fileName(), name)) {
                qInfo() << "[FileManager] textClient.inc gefunden:" << fi.absoluteFilePath();
                return fi.absoluteFilePath();
            }
        }
    }

    qWarning() << "[FileManager] Keine textClient.inc gefunden in:" << baseDir;
    return {};
}

// ------------------------------------------------------
// Suche nach Text-Dateien (restdata.txt, textclient.inc, textclient.txt)
// ------------------------------------------------------
QString FileManager::findTextFile(const QString& layoutFile) const
{
    QFileInfo layoutInfo(layoutFile);
    if (!layoutInfo.exists() || !layoutInfo.isFile()) {
        qWarning() << "[FileManager] Ungültiger Layout-Pfad:" << layoutFile;
        return {};
    }

    const QString baseDir = layoutInfo.absolutePath();

    // Alle möglichen FlyFF-Varianten abdecken
    const QStringList candidates = {
        "textclient.txt",
        "textclient.txt.txt",
        "restdata.txt",
        "restdata.txt.txt"
    };

    QDirIterator it(baseDir, QDir::Files);
    while (it.hasNext()) {
        QFileInfo fi(it.next());
        for (const auto& name : candidates) {
            if (nameEqualsIgnoreCase(fi.fileName(), name)) {
                qInfo() << "[FileManager] Text-Datei gefunden:" << fi.absoluteFilePath();
                return fi.absoluteFilePath();
            }
        }
    }

    qWarning() << "[FileManager] Keine Text-Datei gefunden in:" << baseDir;
    return {};
}

bool FileManager::hasCachedFlags(const QString& configDir) const
{
    return QFileInfo::exists(configDir + "/window_flags.json")
    && QFileInfo::exists(configDir + "/control_flags.json");
}

// -----------------------------
// Define-Dateien suchen
// -----------------------------
QString FileManager::findDefineFile(const QString& layoutFile) const
{
    QFileInfo layoutInfo(layoutFile);
    if (!layoutInfo.exists() || !layoutInfo.isFile()) {
        qWarning().noquote() << "[FileManager] Ungültiger Layout-Pfad:" << layoutFile;
        return {};
    }

    const QString baseDir = layoutInfo.absolutePath();
    const QString target = "resdata.h";

    QDirIterator it(baseDir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QFileInfo fi(it.next());

        // Case-insensitive Vergleich
        if (fi.fileName().compare(target, Qt::CaseInsensitive) == 0) {
            qInfo().noquote() << "[FileManager] Define-Datei gefunden:" << fi.absoluteFilePath();
            return fi.absoluteFilePath();
        }
    }

    qWarning().noquote() << "[FileManager] Keine Define-Datei gefunden in:" << baseDir;
    return {};
}

QString FileManager::layoutPath() const
{
    return m_config ? m_config->layoutPath() : QString();
}

QString FileManager::textPath() const
{
    if (!m_textPath.isEmpty())
        return m_textPath;

    QString layoutPath = m_layoutPath;
    if (layoutPath.isEmpty() && m_config)
        layoutPath = m_config->layoutPath();

    if (layoutPath.isEmpty()) {
        qWarning().noquote() << "[FileManager] Kein Layout-Pfad in Config oder Member!";
        return {};
    }

    QString path = findTextFile(layoutPath);
    if (!path.isEmpty())
        m_textPath = path;

    return m_textPath;
}


QString FileManager::definePath() const
{
    // Falls bereits gefunden → zurückgeben
    if (!m_definePath.isEmpty())
        return m_definePath;

    // Layout-Pfad aus Config oder Member holen
    QString layoutPath = m_layoutPath;
    if (layoutPath.isEmpty() && m_config)
        layoutPath = m_config->layoutPath();

    if (layoutPath.isEmpty()) {
        qWarning().noquote() << "[FileManager] Kein Layout-Pfad in Config oder Member!";
        return {};
    }

    // Suche starten
    QString path = findDefineFile(layoutPath);
    if (!path.isEmpty())
        m_definePath = path; // Cache speichern

    return m_definePath;
}

QJsonObject FileManager::loadJsonObject(const QString& filePath) const
{
    QFile file(filePath);

    if (!file.exists()) {
        qWarning().noquote() << "[FileManager] Datei fehlt:" << filePath;
        return {};
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning().noquote() << "[FileManager] Konnte Datei nicht öffnen:" << filePath;
        return {};
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError) {
        qWarning().noquote()
        << "[FileManager] JSON-Fehler in" << filePath << ":" << err.errorString();
        return {};
    }

    if (!doc.isObject()) {
        qWarning().noquote() << "[FileManager] JSON ist kein Objekt:" << filePath;
        return {};
    }

    return doc.object();
}

QString FileManager::windowFlagRulesPath() const
{
    if (!m_config) {
        qWarning().noquote()
        << "[FileManager] Kein ConfigManager – kann window_flag_rules.json nicht bestimmen.";
        return {};
    }

    // Immer das Standard-Config-Verzeichnis der Anwendung
    const QString dir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(dir);

    const QString path = dir + "/window_flag_rules.json";
    qInfo().noquote() << "[FileManager] windowFlagRulesPath() →" << path;
    return path;
}


QString FileManager::controlFlagRulesPath() const
{
    if (!m_config) {
        qWarning().noquote()
        << "[FileManager] Kein ConfigManager – kann control_flag_rules.json nicht bestimmen.";
        return {};
    }

    const QString dir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(dir);

    const QString path = dir + "/control_flag_rules.json";
    qInfo().noquote() << "[FileManager] controlFlagRulesPath() →" << path;
    return path;
}

QString FileManager::windowFlagsPath() const
{
    if (!m_config) {
        qWarning().noquote() << "[FileManager] Kein ConfigManager – kann window_flags.json nicht bestimmen.";
        return {};
    }

    const QString dir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(dir);

    const QString path = dir + "/window_flags.json";
    qInfo().noquote() << "[FileManager] windowFlagsPath() →" << path;
    return path;
}

QString FileManager::controlFlagsPath() const
{
    if (!m_config) {
        qWarning().noquote() << "[FileManager] Kein ConfigManager – kann control_flags.json nicht bestimmen.";
        return {};
    }

    const QString dir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(dir);

    const QString path = dir + "/control_flags.json";
    qInfo().noquote() << "[FileManager] controlFlagsPath() →" << path;
    return path;
}

QString FileManager::windowTypesPath() const
{
    if (!m_config) {
        qWarning().noquote() << "[FileManager] Kein ConfigManager – kann window_types.json nicht bestimmen.";
        return {};
    }

    const QString dir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(dir);

    const QString path = dir + "/window_types.json";
    qInfo().noquote() << "[FileManager] windowTypesPath() →" << path;
    return path;
}

QString FileManager::undefinedControlFlagsPath() const
{
    if (!m_config) {
        qWarning().noquote() << "[FileManager] Kein ConfigManager – kann undefined_control_flags.json nicht bestimmen.";
        return {};
    }

    const QString dir = QCoreApplication::applicationDirPath() + "/config";
    QDir().mkpath(dir);

    const QString path = dir + "/undefined_control_flags.json";
    qInfo().noquote() << "[FileManager] undefinedControlFlagsPath() →" << path;
    return path;
}

bool FileManager::saveJsonObject(const QString& filePath, const QJsonObject& json) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning().noquote() << "[FileManager] Konnte Datei nicht schreiben:" << filePath;
        return false;
    }

    QJsonDocument doc(json);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    qInfo().noquote() << "[FileManager] JSON gespeichert:" << filePath;
    return true;
}
