#include "FlagManager.h"
#include "ConfigManager.h"
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>

// -------------------------------------------------------------
// Konstruktor
// -------------------------------------------------------------
FlagManager::FlagManager(ConfigManager* config)
    : m_config(config)
{
    qInfo() << "[FlagManager] Initialisiert.";
}

// ============================================================================
// Hilfsfunktion: Numerische Werte -> Hexstring (0xXXXXXXXX)
// ============================================================================
static QString toHex(const QString& valStr)
{
    QString s = valStr.trimmed();
    quint32 value = 0;

    // Bitshift: (1 << x)
    QRegularExpression shiftRe(R"(\(?\s*1\s*<<\s*(\d+)\s*\)?)");
    QRegularExpressionMatch sm = shiftRe.match(s);
    if (sm.hasMatch()) {
        value = (1u << sm.captured(1).toUInt());
    }
    // Bitmasken mit OR
    else if (s.contains("|")) {
        QStringList parts = s.split("|");
        for (QString p : parts)
            value |= p.trimmed().startsWith("0x", Qt::CaseInsensitive)
                         ? p.trimmed().toUInt(nullptr, 16)
                         : p.trimmed().toUInt(nullptr, 10);
    }
    // Kombination mit +
    else if (s.contains("+")) {
        QStringList parts = s.split("+");
        for (QString p : parts)
            value += p.trimmed().startsWith("0x", Qt::CaseInsensitive)
                         ? p.trimmed().toUInt(nullptr, 16)
                         : p.trimmed().toUInt(nullptr, 10);
    }
    // Hexwert
    else if (s.startsWith("0x", Qt::CaseInsensitive)) {
        value = s.toUInt(nullptr, 16);
    }
    // Plain Decimal
    else if (s.contains(QRegularExpression("^[0-9]+$"))) {
        value = s.toUInt(nullptr, 10);
    }

    return QString("0x%1").arg(value, 8, 16, QLatin1Char('0')).toUpper();
}

// ============================================================================
// Lambda: erkennt automatisch, ob WindowFlag, ControlFlag oder WindowType
// ============================================================================
static void classifyAndInsert(
    QMap<QString, QString>& windowFlags,
    QMap<QString, QString>& controlFlags,
    QMap<QString, QString>& windowTypes,
    const QString& name, const QString& hexVal)
{
    static const QStringList windowPrefixes = {
        "WBS_", "WLVS_", "WSS_", "WCS_", "WVS_", "WFS_", "WNS_", "WMS_"
    };
    static const QStringList controlPrefixes = {
        "BS_", "EBS_", "CBS_", "LBS_", "SBS_", "TBS_", "MBS_", "RBS_", "TCS_",
        "PBS_", "GBS_", "FBS_", "HBS_", "DBS_", "UBS_", "KBS_"
    };

    for (const QString& p : windowPrefixes)
        if (name.startsWith(p)) {
            windowFlags.insert(name, hexVal);
            return;
        }

    for (const QString& p : controlPrefixes)
        if (name.startsWith(p)) {
            controlFlags.insert(name, hexVal);
            return;
        }

    windowTypes.insert(name, hexVal);
}

// ============================================================================
// Scannt eine Header-Datei und extrahiert alle relevanten Flag-Definitionen
// ============================================================================
void FlagManager::parseHeaderFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&file);
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (!line.startsWith("#define"))
            continue;

        const QStringList tokens = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (tokens.size() < 3)
            continue;

        QString key   = tokens[1];
        QString value = tokens[2].trimmed();

        // 🔹 Entferne Endungen wie "L", "UL", "U"
        value.remove(QRegularExpression("[lLuU]+$"));

        // 🔹 Zahlenwert in Hex normalisieren
        if (!value.startsWith("0x", Qt::CaseInsensitive)) {
            bool ok = false;
            quint32 dec = value.toUInt(&ok, 10);
            if (ok)
                value = QString("0x%1").arg(dec, 8, 16, QLatin1Char('0')).toUpper();
            else
                continue;
        } else {
            bool ok = false;
            value.toUInt(&ok, 16);
            if (!ok)
                continue;
            value = value.toUpper();
        }

        // 🔸 Gruppierung
        if      (key.startsWith("WBS_"))   m_windowFlags.insert(key, value);
        else if (key.startsWith("BS_"))    m_controlFlags.insert(key, value);
        else if (key.startsWith("EBS_"))   m_controlFlags.insert(key, value);
        else if (key.startsWith("TCS_"))   m_controlFlags.insert(key, value);
        else if (key.startsWith("WLVS_"))  m_controlFlags.insert(key, value);
        else if (key.startsWith("SS_"))    m_controlFlags.insert(key, value);
        else if (key.startsWith("WTYPE_")) m_windowTypes.insert(key, value);
    }
}
// ============================================================================
// Generiert alle Flags aus den angegebenen Header-Pfaden (rekursiv)
// ============================================================================
void FlagManager::generateFlags(const QString& sourceDir,
                                const QString& wndPath,
                                const QString& ctrlPath)
{
    m_windowFlags.clear();
    m_controlFlags.clear();
    m_windowTypes.clear();
    m_unknownFlags.clear();

    // 🔹 Source-Ordner prüfen
    if (sourceDir.isEmpty() || !QDir(sourceDir).exists()) {
        qWarning() << "[FlagManager] Ungültiger Source-Pfad:" << sourceDir;
        return;
    }

    qInfo().noquote() << QString("[FlagManager] Starte Header-Scan in: %1").arg(sourceDir);

    // 🔍 Alle Headerdateien rekursiv durchsuchen
    QStringList headerFiles;
    QDirIterator it(sourceDir, QStringList() << "*.h", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        headerFiles << it.next();

    qInfo().noquote() << QString("[FlagManager] %1 Header-Dateien gefunden.").arg(headerFiles.size());

    // 🔄 Alle Headerdateien parsen
    for (const QString& filePath : headerFiles)
        parseHeaderFile(filePath);

    // 🔹 HIER — nach dem Scan, bevor die Dateien geschrieben werden
    applyDefaultWindowFlags();
    applyDefaultWindowTypes();

    // 🧾 Statistik ausgeben
    qInfo().noquote()
        << QString("[FlagManager] Parsing abgeschlossen → WindowFlags: %1, ControlFlags: %2, Types: %3, Unknown: %4")
               .arg(m_windowFlags.size())
               .arg(m_controlFlags.size())
               .arg(m_windowTypes.size())
               .arg(m_unknownFlags.size());

    // 💾 Helper zum JSON-Speichern
    auto saveJsonFile = [](const QString& path, const QMap<QString, QString>& map, const QString& label)
    {
        if (map.isEmpty()) {
            qWarning() << "[FlagManager]" << label << "→ Keine Daten, Datei wird übersprungen:" << path;
            return;
        }

        QJsonObject obj;
        for (auto it = map.constBegin(); it != map.constEnd(); ++it)
            obj.insert(it.key(), it.value());

        QFile f(path);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
            f.close();
            qInfo() << "[FlagManager] Gespeichert:" << path;
        } else {
            qWarning() << "[FlagManager] Fehler beim Schreiben:" << path;
        }
    };

    // 🔹 Dateien speichern
    saveJsonFile(wndPath, m_windowFlags, "WindowFlags");
    saveJsonFile(ctrlPath, m_controlFlags, "ControlFlags");

    QString typePath = QFileInfo(wndPath).absolutePath() + "/window_types.json";
    saveJsonFile(typePath, m_windowTypes, "WindowTypes");

    qInfo() << "[FlagManager] Flags erfolgreich generiert.";
}

// -------------------------------------------------------------
// Standard-Window-Flags ergänzen (wenn nicht vorhanden)
// -------------------------------------------------------------
void FlagManager::applyDefaultWindowFlags()
{
    if (!m_windowFlags.isEmpty()) {
        qInfo() << "[FlagManager] WindowFlags bereits vorhanden, Defaults werden übersprungen.";
        return;
    }

    qInfo() << "[FlagManager] Wende Default Window Base Style Flags an...";

    m_windowFlags.insert("WBS_CAPTION",       "0X02000000");
    m_windowFlags.insert("WBS_CHECK",         "0X00000008");
    m_windowFlags.insert("WBS_CHILD",         "0X00020000");
    m_windowFlags.insert("WBS_CHILDFRAME",    "0X00800000");
    m_windowFlags.insert("WBS_DOCKING",       "0X04000000");
    m_windowFlags.insert("WBS_EXTENSION",     "0X00000020");
    m_windowFlags.insert("WBS_HELP",          "0X00000004");
    m_windowFlags.insert("WBS_HIGHLIGHT",     "0X00000010");
    m_windowFlags.insert("WBS_HIGHLIGHTPUSH", "0X00000020");
    m_windowFlags.insert("WBS_HORI",          "0X00000001");
    m_windowFlags.insert("WBS_HSCROLL",       "0X40000000");
    m_windowFlags.insert("WBS_KEY",           "0X01000000");
    m_windowFlags.insert("WBS_MANAGER",       "0X00100000");
    m_windowFlags.insert("WBS_MAXIMIZEBOX",   "0X00000002");
    m_windowFlags.insert("WBS_MENUITEM",      "0X00000100");
    m_windowFlags.insert("WBS_MINIMIZEBOX",   "0X00000001");
    m_windowFlags.insert("WBS_MODAL",         "0X00080000");
    m_windowFlags.insert("WBS_MONEY",         "0X00000004");
    m_windowFlags.insert("WBS_MOVE",          "0X00010000");
    m_windowFlags.insert("WBS_NOCENTER",      "0X00000080");
    m_windowFlags.insert("WBS_NODRAWFRAME",   "0X00040000");
    m_windowFlags.insert("WBS_NOFOCUS",       "0X80000000");
    m_windowFlags.insert("WBS_NOFRAME",       "0X00200000");
    m_windowFlags.insert("WBS_NOMENUICON",    "0X00000400");
    m_windowFlags.insert("WBS_OVERRIDE_FIRST","0X00000040");
    m_windowFlags.insert("WBS_PIN",           "0X00000010");
    m_windowFlags.insert("WBS_POPUP",         "0X08000000");
    m_windowFlags.insert("WBS_PUSHLIKE",      "0X00000200");
    m_windowFlags.insert("WBS_RADIO",         "0X00000004");
    m_windowFlags.insert("WBS_SOUND",         "0X00400000");
    m_windowFlags.insert("WBS_SPRITE",        "0X00000002");
    m_windowFlags.insert("WBS_TEXT",          "0X00000001");
    m_windowFlags.insert("WBS_THICKFRAME",    "0X00000040");
    m_windowFlags.insert("WBS_TOPMOST",       "0X10000000");
    m_windowFlags.insert("WBS_VERT",          "0X00000002");
    m_windowFlags.insert("WBS_VIEW",          "0X00000008");
    m_windowFlags.insert("WBS_VSCROLL",       "0X20000000");

    qInfo() << "[FlagManager] Default WindowFlags hinzugefügt:" << m_windowFlags.size();
}


// -------------------------------------------------------------
// Standard Window Types ergänzen
// -------------------------------------------------------------
void FlagManager::applyDefaultWindowTypes()
{
    static const QMap<QString, QString> defaults = {
        {"WTYPE_NONE",      "0x00000000"},
        {"WTYPE_BASE",      "0x00000001"},
        {"WTYPE_STATIC",    "0x00000002"},
        {"WTYPE_BUTTON",    "0x00000003"},
        {"WTYPE_EDIT",      "0x00000004"},
        {"WTYPE_SCROLLBAR", "0x00000005"},
        {"WTYPE_LISTBOX",   "0x00000006"},
        {"WTYPE_CUSTOM",    "0x00000007"}
    };

    for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it) {
        if (!m_windowTypes.contains(it.key())) {
            m_windowTypes[it.key()] = it.value();
            qInfo() << "[FlagManager] Default Window-Type ergänzt:" << it.key() << "=" << it.value();
        }
    }
}

// -------------------------------------------------------------
// JSON speichern (generisch)
// -------------------------------------------------------------
void FlagManager::saveJsonFile(const QString& path,
                               const QMap<QString, QString>& data,
                               const QString& label)
{
    QJsonObject obj;
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        QString val = it.value().trimmed().toUpper();

        // 🔹 Sicherstellen, dass der Wert ein Hexstring ist
        if (!val.startsWith("0X")) {
            bool ok = false;
            quint32 num = val.toUInt(&ok, 16);
            if (ok)
                val = QString("0X%1").arg(num, 8, 16, QLatin1Char('0')).toUpper();
            else
                val = QString("0X00000000");
        } else {
            // Einheitliche Schreibweise (0X + 8-stellig)
            val.remove("L");
            bool ok = false;
            quint32 num = val.mid(2).toUInt(&ok, 16);
            if (ok)
                val = QString("0X%1").arg(num, 8, 16, QLatin1Char('0')).toUpper();
        }

        obj.insert(it.key(), val);
    }

    QJsonDocument doc(obj);

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        qInfo().noquote() << QString("[FlagManager] %1-Flags gespeichert → %2")
                                 .arg(label, QDir::toNativeSeparators(path));
    } else {
        qWarning() << "[FlagManager] Fehler beim Schreiben von" << path;
    }
}
