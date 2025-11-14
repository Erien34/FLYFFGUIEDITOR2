#include "FlagManager.h"
#include "ConfigManager.h"
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QDebug>
#include <qjsonarray.h>

// -------------------------------------------------------------
// Konstruktor
// -------------------------------------------------------------
FlagManager::FlagManager(ConfigManager* config)
    : m_config(config)
{
    m_legacyOverrides = {
        "WBS_NOCLOSE"
    };
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

    // Defaults injizieren
    applyDefaultWindowFlags();
    applyDefaultWindowTypes();
    applyDefaultControlFlags();

    // 🧾 Statistik
    qInfo().noquote()
        << QString("[FlagManager] Parsing abgeschlossen → WindowFlags: %1, ControlFlags: %2, Types: %3, Unknown: %4")
               .arg(m_windowFlags.size())
               .arg(m_controlFlags.size())
               .arg(m_windowTypes.size())
               .arg(m_unknownFlags.size());

    // 💾 Helper zum JSON-Speichern
    auto saveJsonFile = [](const QString& path, const QMap<QString, QString>& map, const QString& label)
    {
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
    saveJsonFile(wndPath,  m_windowFlags,  "WindowFlags");
    saveJsonFile(ctrlPath, m_controlFlags, "ControlFlags");

    QString typePath = QFileInfo(wndPath).absolutePath() + "/window_types.json";
    saveJsonFile(typePath, m_windowTypes, "WindowTypes");

    qInfo() << "[FlagManager] Flags erfolgreich generiert.";

    //
    // IMPORTANT PART (was previously missing)
    //
    const QString configDir = QFileInfo(wndPath).absolutePath();

    // 1.) Rule templates erzeugen
    generateRuleTemplates(configDir);

    // 2.) Group file erzeugen (jetzt hat der Manager alle Flag-Daten!)
    generateFlagGroups(configDir);

    // 3.) Fehlende Flags ergänzen
    extendFlagGroups(configDir);
}
void FlagManager::generateRuleTemplates(const QString& configDir)
{
    const QString wndRulePath  = configDir + "/window_flag_rules.json";
    const QString ctrlRulePath = configDir + "/control_flag_rules.json";

    // 0️⃣ Semantik MUSS zuerst geladen werden
    initDefaultSemantics();

    // ================================
    // 1️⃣ WINDOW FLAG RULE TEMPLATE
    // ================================
    if (!QFile::exists(wndRulePath))
    {
        qInfo() << "[FlagManager] Erzeuge window_flag_rules.json ...";

        QJsonObject root;

        for (auto it = m_windowFlags.begin(); it != m_windowFlags.end(); ++it)
        {
            QJsonObject rule;
            rule["set"] = QJsonObject{};
            root.insert(it.key(), rule);
        }

        QFile f(wndRulePath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            f.close();
            qInfo() << "[FlagManager] window_flag_rules.json erzeugt.";
        }
        else qWarning() << "[FlagManager] Konnte window_flag_rules.json nicht schreiben!";
    }

    // ================================
    // 2️⃣ CONTROL FLAG RULE TEMPLATE
    // ================================
    if (!QFile::exists(ctrlRulePath))
    {
        qInfo() << "[FlagManager] Erzeuge control_flag_rules.json ...";

        QJsonObject root;

        for (auto it = m_controlFlags.begin(); it != m_controlFlags.end(); ++it)
        {
            QJsonObject rule;
            rule["set"] = QJsonObject{};
            root.insert(it.key(), rule);
        }

        QFile f(ctrlRulePath);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
            f.close();
            qInfo() << "[FlagManager] control_flag_rules.json erzeugt.";
        }
        else qWarning() << "[FlagManager] Konnte control_flag_rules.json nicht schreiben!";
    }

    // 3️⃣ fehlende Flags ergänzen
    extendRuleFile(wndRulePath,  m_windowFlags);
    extendRuleFile(ctrlRulePath, m_controlFlags);

    // 4️⃣ Default Semantik in Rule-Dateien übertragen
    autoFillSemantics(wndRulePath);
    autoFillSemantics(ctrlRulePath);
}

void FlagManager::autoFillSemantics(const QString& ruleFilePath)
{
    QFile file(ruleFilePath);
    if (!file.exists()) {
        qWarning() << "[FlagManager][autoFillSemantics] Datei existiert nicht:" << ruleFilePath;
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FlagManager][autoFillSemantics] Konnte Datei nicht öffnen:" << ruleFilePath;
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);

    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[FlagManager][autoFillSemantics] Ungültiges JSON:" << err.errorString();
        return;
    }

    QJsonObject root = doc.object();

    //
    // STARTE SEMANTIK-ERGÄNZUNG
    //
    for (auto it = m_defaultSemantics.begin(); it != m_defaultSemantics.end(); ++it)
    {
        const QString flag = it.key();
        const QJsonObject defaultObj = it.value();

        // Falls Flag nicht in der Datei existiert → füge vollen Satz ein
        if (!root.contains(flag))
        {
            QJsonObject ruleEntry;
            ruleEntry["set"] = defaultObj;
            root.insert(flag, ruleEntry);
            continue;
        }

        // Wenn Flag existiert → ergänze fehlende Werte
        QJsonObject entry = root.value(flag).toObject();
        QJsonObject setObj = entry.value("set").toObject();

        for (auto dIt = defaultObj.begin(); dIt != defaultObj.end(); ++dIt)
        {
            const QString key = dIt.key();

            // Nur ergänzen, NICHT überschreiben!
            if (!setObj.contains(key))
                setObj.insert(key, dIt.value());
        }

        entry["set"] = setObj;
        root[flag] = entry;
    }

    //
    // JSON zurückschreiben
    //
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    qInfo().noquote() << "[FlagManager] autoFillSemantics abgeschlossen für:"
                      << QFileInfo(ruleFilePath).fileName();
}

void FlagManager::extendRuleFile(const QString& filePath,
                                 const QMap<QString, QString>& knownFlags)
{
    QFile f(filePath);

    // -----------------------------
    // Datei existiert? Wenn nicht → ignorieren
    // (Phase 1 erzeugt die Datei)
    // -----------------------------
    if (!f.exists()) {
        qWarning() << "[FlagManager] extendRuleFile: Datei existiert nicht:" << filePath;
        return;
    }

    QJsonObject root;

    // -----------------------------
    // Datei laden
    // -----------------------------
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray data = f.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        root = doc.object();
        f.close();
    }

    bool changed = false;

    // -----------------------------
    // Flags vergleichen → fehlende Regeln einfügen
    // -----------------------------
    for (auto it = knownFlags.begin(); it != knownFlags.end(); ++it)
    {
        QString flagName = it.key();

        if (!root.contains(flagName))
        {
            // Neue Regel erzeugen
            QJsonObject rule;
            rule["set"] = QJsonObject{};  // leeres Objekt

            root.insert(flagName, rule);

            changed = true;

            qInfo() << "[FlagManager] ➕ Neue Regel hinzugefügt für:" << flagName;
        }
    }

    // -----------------------------
    // Keine Änderungen? → nichts speichern
    // -----------------------------
    if (!changed)
        return;

    // -----------------------------
    // Änderungen speichern
    // -----------------------------
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
        qInfo() << "[FlagManager] Aktualisiert:" << filePath;
    } else {
        qWarning() << "[FlagManager] Fehler beim Schreiben:" << filePath;
    }
}

void FlagManager::initDefaultSemantics()
{
    m_defaultSemantics.clear();

    auto add = [&](const QString& flag, const QJsonObject& obj) {
        m_defaultSemantics.insert(flag, obj);
    };

    QJsonObject o;

    // BS_PUSHBUTTON
    o = QJsonObject();
    o["role"] = "button";
    o["toggle"] = false;
    add("BS_PUSHBUTTON", o);

    // BS_DEFPUSHBUTTON
    o = QJsonObject();
    o["role"] = "button";
    o["default"] = true;
    add("BS_DEFPUSHBUTTON", o);

    // BS_CHECKBOX
    o = QJsonObject();
    o["role"] = "checkbox";
    o["toggle"] = true;
    o["triState"] = false;
    add("BS_CHECKBOX", o);

    // BS_AUTOCHECKBOX
    o = QJsonObject();
    o["role"] = "checkbox";
    o["toggle"] = true;
    o["autoToggle"] = true;
    add("BS_AUTOCHECKBOX", o);

    // BS_3STATE
    o = QJsonObject();
    o["role"] = "checkbox";
    o["toggle"] = true;
    o["triState"] = true;
    add("BS_3STATE", o);

    // BS_AUTO3STATE
    o = QJsonObject();
    o["role"] = "checkbox";
    o["toggle"] = true;
    o["triState"] = true;
    o["autoToggle"] = true;
    add("BS_AUTO3STATE", o);

    // BS_RADIOBUTTON
    o = QJsonObject();
    o["role"] = "radiobutton";
    o["toggle"] = true;
    o["exclusive"] = true;
    o["groupMember"] = true;
    add("BS_RADIOBUTTON", o);

    // BS_AUTORADIOBUTTON
    o = QJsonObject();
    o["role"] = "radiobutton";
    o["toggle"] = true;
    o["exclusive"] = true;
    o["groupMember"] = true;
    o["autoToggle"] = true;
    add("BS_AUTORADIOBUTTON", o);

    // BS_GROUPBOX
    o = QJsonObject();
    o["role"] = "groupbox";
    o["container"] = true;
    add("BS_GROUPBOX", o);

    // BS_BITMAP
    o = QJsonObject();
    o["role"] = "picturebutton";
    o["usesBitmap"] = true;
    add("BS_BITMAP", o);

    // BS_ICON
    o = QJsonObject();
    o["role"] = "picturebutton";
    o["usesIcon"] = true;
    add("BS_ICON", o);

    // BS_LEFT
    o = QJsonObject();
    o["contentAlign"] = "left";
    add("BS_LEFT", o);

    // BS_RIGHT
    o = QJsonObject();
    o["contentAlign"] = "right";
    add("BS_RIGHT", o);

    // BS_TOP
    o = QJsonObject();
    o["contentVAlign"] = "top";
    add("BS_TOP", o);

    // BS_BOTTOM
    o = QJsonObject();
    o["contentVAlign"] = "bottom";
    add("BS_BOTTOM", o);

    // BS_VCENTER
    o = QJsonObject();
    o["contentVAlign"] = "center";
    add("BS_VCENTER", o);

    // BS_NONE
    o = QJsonObject();
    o["role"] = "none";
    add("BS_NONE", o);

    // BS_MODEL
    o = QJsonObject();
    o["role"] = "model";
    add("BS_MODEL", o);

    // BS_OBJECT
    o = QJsonObject();
    o["role"] = "object";
    add("BS_OBJECT", o);

    // BS_MTRLBLK
    o = QJsonObject();
    o["role"] = "materialblock";
    add("BS_MTRLBLK", o);


    // EBS_AUTOHSCROLL
    o = QJsonObject();
    o["autoHScroll"] = true;
    o["scrollable"] = true;
    add("EBS_AUTOHSCROLL", o);

    // EBS_AUTOVSCROLL
    o = QJsonObject();
    o["autoVScroll"] = true;
    o["scrollable"] = true;
    add("EBS_AUTOVSCROLL", o);

    // EBS_CENTER
    o = QJsonObject();
    o["textAlign"] = "center";
    add("EBS_CENTER", o);

    // EBS_LEFT
    o = QJsonObject();
    o["textAlign"] = "left";
    add("EBS_LEFT", o);

    // EBS_LOWERCASE
    o = QJsonObject();
    o["forceLowercase"] = true;
    add("EBS_LOWERCASE", o);

    // EBS_MULTILINE
    o = QJsonObject();
    o["multiline"] = true;
    add("EBS_MULTILINE", o);

    // EBS_NOHIDESEL
    o = QJsonObject();
    o["keepSelectionVisible"] = true;
    add("EBS_NOHIDESEL", o);

    // EBS_NUMBER
    o = QJsonObject();
    o["numeric"] = true;
    add("EBS_NUMBER", o);

    // EBS_OEMCONVERT
    o = QJsonObject();
    o["oemConvert"] = true;
    add("EBS_OEMCONVERT", o);

    // EBS_PASSWORD
    o = QJsonObject();
    o["masked"] = true;
    add("EBS_PASSWORD", o);

    // EBS_READONLY
    o = QJsonObject();
    o["readonly"] = true;
    add("EBS_READONLY", o);

    // EBS_RIGHT
    o = QJsonObject();
    o["textAlign"] = "right";
    add("EBS_RIGHT", o);

    // EBS_UPPERCASE
    o = QJsonObject();
    o["forceUppercase"] = true;
    add("EBS_UPPERCASE", o);

    // EBS_WANTRETURN
    o = QJsonObject();
    o["acceptReturn"] = true;
    add("EBS_WANTRETURN", o);

    // ES_AUTOHSCROLL
    o = QJsonObject();
    o["autoHScroll"] = true;
    o["scrollable"] = true;
    add("ES_AUTOHSCROLL", o);

    // ES_AUTOVSCROLL
    o = QJsonObject();
    o["autoVScroll"] = true;
    o["scrollable"] = true;
    add("ES_AUTOVSCROLL", o);

    // ES_CENTER
    o = QJsonObject();
    o["textAlign"] = "center";
    add("ES_CENTER", o);

    // ES_LEFT
    o = QJsonObject();
    o["textAlign"] = "left";
    add("ES_LEFT", o);

    // ES_MULTILINE
    o = QJsonObject();
    o["multiline"] = true;
    add("ES_MULTILINE", o);

    // ES_NOHIDESEL
    o = QJsonObject();
    o["keepSelectionVisible"] = true;
    add("ES_NOHIDESEL", o);

    // ES_NUMBER
    o = QJsonObject();
    o["numeric"] = true;
    add("ES_NUMBER", o);

    // ES_OEMCONVERT
    o = QJsonObject();
    o["oemConvert"] = true;
    add("ES_OEMCONVERT", o);

    // ES_PASSWORD
    o = QJsonObject();
    o["masked"] = true;
    add("ES_PASSWORD", o);

    // ES_READONLY
    o = QJsonObject();
    o["readonly"] = true;
    add("ES_READONLY", o);

    // ES_RIGHT
    o = QJsonObject();
    o["textAlign"] = "right";
    add("ES_RIGHT", o);

    // ES_WANTRETURN
    o = QJsonObject();
    o["acceptReturn"] = true;
    add("ES_WANTRETURN", o);

    // LBS_DISABLENOSCROLL
    o = QJsonObject();
    o["disableNoScroll"] = true;
    add("LBS_DISABLENOSCROLL", o);

    // LBS_EXTENDEDSEL
    o = QJsonObject();
    o["extendedSelect"] = true;
    add("LBS_EXTENDEDSEL", o);

    // LBS_HASSTRINGS
    o = QJsonObject();
    o["hasStrings"] = true;
    add("LBS_HASSTRINGS", o);

    // LBS_MULTICOLUMN
    o = QJsonObject();
    o["multiColumn"] = true;
    add("LBS_MULTICOLUMN", o);

    // LBS_MULTIPLESEL
    o = QJsonObject();
    o["multiSelect"] = true;
    add("LBS_MULTIPLESEL", o);

    // LBS_NOINTEGRALHEIGHT
    o = QJsonObject();
    o["noIntegralHeight"] = true;
    add("LBS_NOINTEGRALHEIGHT", o);

    // LBS_NOREDRAW
    o = QJsonObject();
    o["noRedraw"] = true;
    add("LBS_NOREDRAW", o);

    // LBS_NOTIFY
    o = QJsonObject();
    o["notify"] = true;
    add("LBS_NOTIFY", o);

    // LBS_OWNERDRAWFIXED
    o = QJsonObject();
    o["ownerDraw"] = true;
    o["itemHeightFixed"] = true;
    add("LBS_OWNERDRAWFIXED", o);

    // LBS_OWNERDRAWVARIABLE
    o = QJsonObject();
    o["ownerDraw"] = true;
    o["itemHeightVariable"] = true;
    add("LBS_OWNERDRAWVARIABLE", o);

    // LBS_SORT
    o = QJsonObject();
    o["sorted"] = true;
    add("LBS_SORT", o);

    // LBS_USETABSTOPS
    o = QJsonObject();
    o["useTabStops"] = true;
    add("LBS_USETABSTOPS", o);

    // LBS_WANTKEYBOARDINPUT
    o = QJsonObject();
    o["wantKeyboardInput"] = true;
    add("LBS_WANTKEYBOARDINPUT", o);

    // SS_BITMAP
    o = QJsonObject();
    o["role"] = "picture";
    o["usesBitmap"] = true;
    add("SS_BITMAP", o);

    // SS_CENTER
    o = QJsonObject();
    o["textAlign"] = "center";
    add("SS_CENTER", o);

    // SS_ICON
    o = QJsonObject();
    o["role"] = "picture";
    o["usesIcon"] = true;
    add("SS_ICON", o);

    // SS_LEFT
    o = QJsonObject();
    o["textAlign"] = "left";
    add("SS_LEFT", o);

    // SS_NOTIFY
    o = QJsonObject();
    o["clickable"] = true;
    add("SS_NOTIFY", o);

    // SS_RIGHT
    o = QJsonObject();
    o["textAlign"] = "right";
    add("SS_RIGHT", o);

    // WLVS_ALIGNLEFT
    o = QJsonObject();
    o["align"] = "left";
    add("WLVS_ALIGNLEFT", o);

    // WLVS_ALIGNMASK
    o = QJsonObject();
    o["alignMask"] = true;
    add("WLVS_ALIGNMASK", o);

    // WLVS_ALIGNTOP
    o = QJsonObject();
    o["align"] = "top";
    add("WLVS_ALIGNTOP", o);

    // WLVS_AUTOARRANGE
    o = QJsonObject();
    o["autoArrange"] = true;
    add("WLVS_AUTOARRANGE", o);

    // WLVS_EDITLABELS
    o = QJsonObject();
    o["editableLabels"] = true;
    add("WLVS_EDITLABELS", o);

    // WLVS_ICON
    o = QJsonObject();
    o["view"] = "icon";
    add("WLVS_ICON", o);

    // WLVS_LIST
    o = QJsonObject();
    o["view"] = "list";
    add("WLVS_LIST", o);

    // WLVS_NOCOLUMNHEADER
    o = QJsonObject();
    o["noColumnHeader"] = true;
    add("WLVS_NOCOLUMNHEADER", o);

    // WLVS_NOLABELWRAP
    o = QJsonObject();
    o["noLabelWrap"] = true;
    add("WLVS_NOLABELWRAP", o);

    // WLVS_NOSCROLL
    o = QJsonObject();
    o["noScroll"] = true;
    add("WLVS_NOSCROLL", o);

    // WLVS_NOSORTHEADER
    o = QJsonObject();
    o["noSortHeader"] = true;
    add("WLVS_NOSORTHEADER", o);

    // WLVS_OWNERDRAWFIXED
    o = QJsonObject();
    o["ownerDraw"] = true;
    add("WLVS_OWNERDRAWFIXED", o);

    // WLVS_REPORT
    o = QJsonObject();
    o["view"] = "report";
    add("WLVS_REPORT", o);

    // WLVS_SHAREIMAGELISTS
    o = QJsonObject();
    o["shareImageLists"] = true;
    add("WLVS_SHAREIMAGELISTS", o);

    // WLVS_SHOWSELALWAYS
    o = QJsonObject();
    o["showSelectionAlways"] = true;
    add("WLVS_SHOWSELALWAYS", o);

    // WLVS_SINGLESEL
    o = QJsonObject();
    o["singleSelection"] = true;
    add("WLVS_SINGLESEL", o);

    // WLVS_SMALLICON
    o = QJsonObject();
    o["view"] = "smallicon";
    add("WLVS_SMALLICON", o);

    // WLVS_SORTASCENDING
    o = QJsonObject();
    o["sortOrder"] = "ascending";
    add("WLVS_SORTASCENDING", o);

    // WLVS_SORTDESCENDING
    o = QJsonObject();
    o["sortOrder"] = "descending";
    add("WLVS_SORTDESCENDING", o);

    // WLVS_TYPEMASK
    o = QJsonObject();
    o["typeMask"] = true;
    add("WLVS_TYPEMASK", o);

    // WLVS_TYPESTYLEMASK
    o = QJsonObject();
    o["typeStyleMask"] = true;
    add("WLVS_TYPESTYLEMASK", o);

    // WBS_CAPTION
    o = QJsonObject();
    o["hasCaption"] = true;
    add("WBS_CAPTION", o);

    // WBS_CHILD
    o = QJsonObject();
    o["isChild"] = true;
    add("WBS_CHILD", o);

    // WBS_CHILDFRAME
    o = QJsonObject();
    o["isChildFrame"] = true;
    add("WBS_CHILDFRAME", o);

    // WBS_DOCKING
    o = QJsonObject();
    o["dockable"] = true;
    add("WBS_DOCKING", o);

    // WBS_EXTENSION
    o = QJsonObject();
    o["hasExtension"] = true;
    add("WBS_EXTENSION", o);

    // WBS_HELP
    o = QJsonObject();
    o["hasHelp"] = true;
    add("WBS_HELP", o);

    // WBS_HSCROLL
    o = QJsonObject();
    o["hasHScroll"] = true;
    add("WBS_HSCROLL", o);

    // WBS_KEY
    o = QJsonObject();
    o["keyWindow"] = true;
    add("WBS_KEY", o);

    // WBS_MANAGER
    o = QJsonObject();
    o["managerWindow"] = true;
    add("WBS_MANAGER", o);

    // WBS_MAXIMIZEBOX
    o = QJsonObject();
    o["maximizeBox"] = true;
    add("WBS_MAXIMIZEBOX", o);

    // WBS_MINIMIZEBOX
    o = QJsonObject();
    o["minimizeBox"] = true;
    add("WBS_MINIMIZEBOX", o);

    // WBS_MODAL
    o = QJsonObject();
    o["modal"] = true;
    add("WBS_MODAL", o);

    // WBS_MOVE
    o = QJsonObject();
    o["draggable"] = true;
    add("WBS_MOVE", o);

    // WBS_NOCENTER
    o = QJsonObject();
    o["noCenter"] = true;
    add("WBS_NOCENTER", o);

    // WBS_NOCLOSE
    o = QJsonObject();
    o["noClose"] = true;
    add("WBS_NOCLOSE", o);

    // WBS_NODRAWFRAME
    o = QJsonObject();
    o["noDrawFrame"] = true;
    add("WBS_NODRAWFRAME", o);

    // WBS_NOFOCUS
    o = QJsonObject();
    o["noFocus"] = true;
    add("WBS_NOFOCUS", o);

    // WBS_NOFRAME
    o = QJsonObject();
    o["borderless"] = true;
    add("WBS_NOFRAME", o);

    // WBS_NOMENUICON
    o = QJsonObject();
    o["noMenuIcon"] = true;
    add("WBS_NOMENUICON", o);

    // WBS_OVERRIDE_FIRST
    o = QJsonObject();
    o["overrideFirst"] = true;
    add("WBS_OVERRIDE_FIRST", o);

    // WBS_PIN
    o = QJsonObject();
    o["pinnable"] = true;
    add("WBS_PIN", o);

    // WBS_POPUP
    o = QJsonObject();
    o["popup"] = true;
    add("WBS_POPUP", o);

    // WBS_RESIZEABLE
    o = QJsonObject();
    o["resizable"] = true;
    add("WBS_RESIZEABLE", o);

    // WBS_SOUND
    o = QJsonObject();
    o["sound"] = true;
    add("WBS_SOUND", o);

    // WBS_THICKFRAME
    o = QJsonObject();
    o["thickFrame"] = true;
    o["resizable"] = true;
    add("WBS_THICKFRAME", o);

    // WBS_TOPMOST
    o = QJsonObject();
    o["topmost"] = true;
    add("WBS_TOPMOST", o);

    // WBS_VIEW
    o = QJsonObject();
    o["viewWindow"] = true;
    add("WBS_VIEW", o);

    // WBS_VSCROLL
    o = QJsonObject();
    o["hasVScroll"] = true;
    add("WBS_VSCROLL", o);

}

void FlagManager::generateFlagGroups(const QString& configDir)
{
    QString path = configDir + "/flag_groups.json";

    QJsonObject root;
    QJsonObject windowObj;
    QJsonObject controlObj;

    //
    // 1️⃣ WINDOW FLAG GROUPS (statisch + exklusiv)
    //

    QJsonObject windowFlags;

    auto addWindow = [&](const QString& flag, const QJsonArray& types) {
        windowFlags.insert(flag, types);
    };

    QJsonArray allTypes = QJsonArray{ "WTYPE_ALL" };
    QJsonArray onlyWindow = QJsonArray{ "WTYPE_WINDOW" };
    QJsonArray allControls = QJsonArray{
        "WTYPE_BUTTON","WTYPE_EDIT","WTYPE_LISTBOX","WTYPE_LISTCTRL",
        "WTYPE_TREE","WTYPE_TAB","WTYPE_STATIC","WTYPE_SCROLLBAR","WTYPE_CUSTOM"
    };

    // global gültige Flags
    addWindow("WBS_CHILD", allTypes);
    addWindow("WBS_VISIBLE", allTypes);
    addWindow("WBS_DISABLED", allTypes);
    addWindow("WBS_HSCROLL", allControls);
    addWindow("WBS_VSCROLL", allControls);

    // nur echte Fenster
    addWindow("WBS_CAPTION", onlyWindow);
    addWindow("WBS_TITLE", onlyWindow);
    addWindow("WBS_MOVE", onlyWindow);
    addWindow("WBS_SIZE", onlyWindow);
    addWindow("WBS_BORDER", onlyWindow);
    addWindow("WBS_FRAME", onlyWindow);
    addWindow("WBS_SYSMENU", onlyWindow);
    addWindow("WBS_TOOLWINDOW", onlyWindow);
    addWindow("WBS_THICKFRAME", onlyWindow);
    addWindow("WBS_MODAL", onlyWindow);

    // Controls dürfen viele Flags erben
    addWindow("WBS_NOFRAME", allControls);

    windowObj.insert("flags", windowFlags);

    //
    // Exklusive Regeln
    //
    QJsonObject windowExclusive;
    windowExclusive.insert("align_h", QJsonArray{"WBS_LEFT","WBS_CENTER","WBS_RIGHT"});
    windowExclusive.insert("align_v", QJsonArray{"WBS_TOP","WBS_VCENTER","WBS_BOTTOM"});
    windowExclusive.insert("frame_mode", QJsonArray{"WBS_THICKFRAME","WBS_NOFRAME","WBS_NODRAWFRAME"});

    windowObj.insert("exclusive", windowExclusive);

    //
    // 2️⃣ CONTROL FLAG GROUPS (VOLL AUTOMATISCH)
    //

    // Prefix → Typ
    QMap<QString, QStringList> typeFlagMap;

    typeFlagMap["WTYPE_BUTTON"]   << "BS_";
    typeFlagMap["WTYPE_EDIT"]     << "ES_" << "EBS_";
    typeFlagMap["WTYPE_LISTBOX"]  << "LBS_";
    typeFlagMap["WTYPE_LISTCTRL"] << "WLVS_";
    typeFlagMap["WTYPE_TREE"]     << "WLVS_";
    typeFlagMap["WTYPE_STATIC"]   << "SS_" << "WSS_";

    // Default styles for all controls
    QJsonArray defaultWndFlags = QJsonArray{"WBS_CHILD","WBS_NOFRAME"};

    for (auto it = typeFlagMap.begin(); it != typeFlagMap.end(); ++it)
    {
        QString type = it.key();
        QStringList prefixes = it.value();

        QJsonArray ctrlFlags;

        for (auto f = m_controlFlags.begin(); f != m_controlFlags.end(); ++f)
        {
            QString flag = f.key();

            for (const QString& p : prefixes)
            {
                if (flag.startsWith(p))
                {
                    ctrlFlags.append(flag);
                    break;
                }
            }
        }

        // Build object
        QJsonObject entry;
        entry.insert("windowStyle", defaultWndFlags);
        entry.insert("controlStyle", ctrlFlags);
        controlObj.insert(type, entry);
    }

    //
    // 3️⃣ SPEICHERN
    //
    root.insert("window", windowObj);
    root.insert("control", controlObj);

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
        qInfo() << "[FlagManager] flag_groups.json erzeugt.";
    }
    else {
        qWarning() << "[FlagManager] Fehler beim Schreiben von flag_groups.json";
    }
}

void FlagManager::extendFlagGroups(const QString& groupsPath)
{
    QFile file(groupsPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "[FlagManager] Kann flag_groups.json nicht öffnen:" << groupsPath;
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();

    // alle Flags zusammenführen
    QStringList allFlags = m_controlFlags.keys() + m_windowFlags.keys();

    // Ergänzen
    for (auto it = root.begin(); it != root.end(); ++it)
    {
        QString groupName = it.key();
        QJsonArray arr = it.value().toArray();

        QSet<QString> set;
        for (const QJsonValue& v : arr)
            set.insert(v.toString());

        // automatisch Flags hinzufügen, deren Prefix passt
        for (const QString& f : allFlags)
            if (!set.contains(f) && groupName.contains("Style", Qt::CaseInsensitive))
                set.insert(f);

        // zurück in Array
        QJsonArray newArr;
        for (const QString& f : set)
            newArr.append(f);

        root[groupName] = newArr;
    }

    // speichern
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    qInfo() << "[FlagManager] flag_groups.json erweitert.";
}
// -------------------------------------------------------------
// Standard-Window-Flags ergänzen (wenn nicht vorhanden)
// -------------------------------------------------------------
void FlagManager::applyDefaultWindowFlags()
{
    static const QMap<QString, QString> defaults = {
        {"WBS_CAPTION",       "0X02000000"},
        {"WBS_CHILD",         "0X00020000"},
        {"WBS_CHILDFRAME",    "0X00800000"},
        {"WBS_DOCKING",       "0X04000000"},
        {"WBS_EXTENSION",     "0X00000020"},
        {"WBS_HELP",          "0X00000004"},
        {"WBS_MOVE",          "0X00010000"},
        {"WBS_MODAL",         "0X00080000"},
        {"WBS_NOFOCUS",       "0X80000000"},
        {"WBS_NOFRAME",       "0X00200000"},
        {"WBS_NODRAWFRAME",   "0X00040000"},
        {"WBS_TOPMOST",       "0X10000000"},
        {"WBS_RESIZEABLE",    "0X00000040"},
        {"WBS_THICKFRAME",    "0X00000040"},
        {"WBS_POPUP",         "0X08000000"},
        {"WBS_VSCROLL",       "0X20000000"},
        {"WBS_HSCROLL",       "0X40000000"},
        {"WBS_VIEW",          "0X00000008"},
        {"WBS_PIN",           "0X00000010"},
        {"WBS_MANAGER",       "0X00100000"},
        {"WBS_KEY",           "0X01000000"},
        {"WBS_SOUND",         "0X00400000"},
        {"WBS_NOMENUICON",    "0X00000400"},
        {"WBS_OVERRIDE_FIRST","0X00000040"},
        {"WBS_NOCENTER",      "0X00000080"},
        {"WBS_MAXIMIZEBOX",   "0X00000002"},
        {"WBS_MINIMIZEBOX",   "0X00000001"}
    };

    static const QMap<QString, QString> legacyDefaults = {
        {"WBS_CHECK",         "0X00000008"},
        {"WBS_PUSHLIKE",      "0X00000200"},
        {"WBS_RADIO",         "0X00000004"},
        {"WBS_MONEY",         "0X00000004"},
        {"WBS_TEXT",          "0X00000001"},
        {"WBS_SPRITE",        "0X00000002"},
        {"WBS_MENUITEM",      "0X00000100"},
        {"WBS_HIGHLIGHT",     "0X00000010"},
        {"WBS_HIGHLIGHTPUSH", "0X00000020"},
        {"WBS_NOCLOSE",       "0X00000080"} // eigentlich legacy
    };

    int added = 0;
    int legacy = 0;

    // 🔹 Schritt 1: Legacy mit Override-Behandlung
    for (auto it = legacyDefaults.constBegin(); it != legacyDefaults.constEnd(); ++it) {
        const QString key = it.key();
        const QString value = it.value();

        if (m_legacyOverrides.contains(key)) {
            // Legacy-Flag bleibt aktiv (z. B. NOCLOSE)
            if (!m_windowFlags.contains(key)) {
                m_windowFlags.insert(key, value);
                added++;
                qInfo().noquote() << "[FlagManager] ⚙️ Legacy-Override aktiv:" << key;
            }
            continue;
        }

        // Normales Legacy-Flag speichern
        if (!m_legacyWindowFlags.contains(key)) {
            m_legacyWindowFlags.insert(key, value);
            legacy++;
        }
    }

    // 🔹 Schritt 2: Aktive Defaults einfügen
    for (auto it = defaults.constBegin(); it != defaults.constEnd(); ++it) {
        const QString key = it.key();
        const QString value = it.value();

        if (!m_windowFlags.contains(key)) {
            m_windowFlags.insert(key, value);
            added++;
        }
    }

    // 🔹 Schritt 3: Sortieren (alphabetisch, case-insensitive)
    QStringList sortedKeys = m_windowFlags.keys();
    std::sort(sortedKeys.begin(), sortedKeys.end(), [](const QString& a, const QString& b) {
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });

    QMap<QString, QString> sortedMap;
    for (const QString& key : sortedKeys)
        sortedMap.insert(key, m_windowFlags.value(key));

    m_windowFlags = sortedMap;

    qInfo().noquote() << QString("[FlagManager] WindowFlags hinzugefügt: %1 | Legacy: %2 | Gesamt aktiv: %3")
                             .arg(added)
                             .arg(legacy)
                             .arg(m_windowFlags.size());
}


void FlagManager::applyDefaultControlFlags()
{
    static const QMap<QString, QString> win32Defaults = {
        // 🟦 Button Styles (BS_*)
        {"BS_PUSHBUTTON",     "0x00000000"},
        {"BS_DEFPUSHBUTTON",  "0x00000001"},
        {"BS_CHECKBOX",       "0x00000002"},
        {"BS_AUTOCHECKBOX",   "0x00000003"},
        {"BS_RADIOBUTTON",    "0x00000004"},
        {"BS_3STATE",         "0x00000005"},
        {"BS_AUTO3STATE",     "0x00000006"},
        {"BS_GROUPBOX",       "0x00000007"},
        {"BS_AUTORADIOBUTTON","0x00000009"},
        {"BS_ICON",           "0x00000040"},
        {"BS_BITMAP",         "0x00000080"},
        {"BS_LEFT",           "0x00000100"},
        {"BS_RIGHT",          "0x00000200"},
        {"BS_TOP",            "0x00000400"},
        {"BS_BOTTOM",         "0x00000800"},
        {"BS_VCENTER",        "0x00000C00"},

        // 🟩 Edit Styles (ES_*)
        {"ES_LEFT",           "0x0000"},
        {"ES_CENTER",         "0x0001"},
        {"ES_RIGHT",          "0x0002"},
        {"ES_MULTILINE",      "0x0004"},
        {"ES_PASSWORD",       "0x0020"},
        {"ES_AUTOVSCROLL",    "0x0040"},
        {"ES_AUTOHSCROLL",    "0x0080"},
        {"ES_NOHIDESEL",      "0x0100"},
        {"ES_OEMCONVERT",     "0x0400"},
        {"ES_READONLY",       "0x0800"},
        {"ES_WANTRETURN",     "0x1000"},
        {"ES_NUMBER",         "0x2000"},

        // 🟨 Static Styles (SS_*)
        {"SS_LEFT",           "0x00000000"},
        {"SS_CENTER",         "0x00000001"},
        {"SS_RIGHT",          "0x00000002"},
        {"SS_ICON",           "0x00000003"},
        {"SS_BITMAP",         "0x0000000E"},
        {"SS_NOTIFY",         "0x00000100"},

        // 🟥 ListBox Styles (LBS_*)
        {"LBS_NOTIFY",        "0x0001"},
        {"LBS_SORT",          "0x0002"},
        {"LBS_NOREDRAW",      "0x0004"},
        {"LBS_MULTIPLESEL",   "0x0008"},
        {"LBS_OWNERDRAWFIXED","0x0010"},
        {"LBS_OWNERDRAWVARIABLE","0x0020"},
        {"LBS_HASSTRINGS",    "0x0040"},
        {"LBS_USETABSTOPS",   "0x0080"},
        {"LBS_NOINTEGRALHEIGHT","0x0100"},
        {"LBS_MULTICOLUMN",   "0x0200"},
        {"LBS_WANTKEYBOARDINPUT","0x0400"},
        {"LBS_EXTENDEDSEL",   "0x0800"},
        {"LBS_DISABLENOSCROLL","0x1000"}
    };

    int added = 0;
    for (auto it = win32Defaults.constBegin(); it != win32Defaults.constEnd(); ++it) {
        if (!m_controlFlags.contains(it.key())) {
            m_controlFlags.insert(it.key(), it.value());
            qInfo() << "[FlagManager] Default Win32 Control-Flag ergänzt:"
                    << it.key() << "=" << it.value();
            ++added;
        }
    }

    qInfo() << QString("[FlagManager] Default ControlFlags hinzugefügt: %1 neu, gesamt: %2")
                   .arg(added)
                   .arg(m_controlFlags.size());
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

static QMap<QString, QString> loadFlagFile(const QString& path)
{
    QMap<QString, QString> map;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[FlagManager] Datei nicht gefunden:" << path;
        return map;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isObject()) {
        qWarning() << "[FlagManager] Ungültiges JSON:" << path;
        return map;
    }

    const QJsonObject obj = doc.object();
    for (auto it = obj.begin(); it != obj.end(); ++it)
        map.insert(it.key(), it.value().toString().toUpper());

    qInfo().noquote() << QString("[FlagManager] Geladen: %1 (%2 Einträge)")
                             .arg(QFileInfo(path).fileName())
                             .arg(map.size());

    return map;
}

// ===========================================================
// Legacy-Loader (getrennte Ordnerstruktur)
// ===========================================================
bool FlagManager::loadLegacyFlags(const QString& baseDir)
{
    const QString legacyDir = QDir(baseDir).filePath("legacy");
    if (!QDir(legacyDir).exists()) {
        qWarning() << "[FlagManager] Legacy-Verzeichnis fehlt:" << legacyDir;
        return false;
    }

    const QString wndFile  = QDir(legacyDir).filePath("window_flags_legacy.json");
    const QString ctrlFile = QDir(legacyDir).filePath("control_flags_legacy.json");

    m_legacyWindowFlags  = loadFlagFile(wndFile);
    m_legacyControlFlags = loadFlagFile(ctrlFile);

    return !(m_legacyWindowFlags.isEmpty() && m_legacyControlFlags.isEmpty());
}

// ===========================================================
// Umschalten zwischen Modern und Legacy
// ===========================================================
bool FlagManager::useLegacyMode(bool enabled)
{
    m_useLegacy = enabled;

    if (enabled) {
        qInfo() << "[FlagManager] Legacy-Mode aktiviert: Verwende alte Flag-Dateien.";
        m_windowFlags  = m_legacyWindowFlags;
        m_controlFlags = m_legacyControlFlags;
    } else {
        qInfo() << "[FlagManager] Legacy-Mode deaktiviert: Verwende aktuelle Flags.";
    }

    return true;
}

void FlagManager::saveFlags(const QString& configDir)
{
    const QString normalPath = configDir + "/window_flags.json";
    const QString legacyPath = configDir + "/legacy_window_flags.json";

    saveJsonFile(normalPath, m_windowFlags, "Window");
    saveJsonFile(legacyPath, m_legacyWindowFlags, "Legacy");

    qInfo().noquote() << QString("[FlagManager] Flags gespeichert → Active:%1 | Legacy:%2")
                             .arg(m_windowFlags.size())
                             .arg(m_legacyWindowFlags.size());
}
