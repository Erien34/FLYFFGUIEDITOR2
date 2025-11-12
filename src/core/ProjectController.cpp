#include "core/ProjectController.h"
#include "core/ConfigManager.h"
#include "core/FileManager.h"
#include "define/DefineBackend.h"
#include "define/DefineManager.h"
#include "define/FlagManager.h"
#include "text/TextBackend.h"
#include "text/TextManager.h"
#include "layout/LayoutParser.h"
#include "layout/LayoutBackend.h"
#include "utils/ResourceUtils.h"
#include "layout/model/TokenData.h"
#include "editor/CanvasHandler.h"
#include "ui/WindowPanel.h"
#include "ui/PropertyPanel.h"
#include "render/RenderManager.h"
#include "theme/ThemeManager.h"
#include "behavior/BehaviorManager.h"


#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>

// --------------------------------------------------
// Konstruktor
// --------------------------------------------------
ProjectController::ProjectController(QObject* parent)
    : QObject(parent),
    m_configManager(std::make_unique<ConfigManager>()),
    m_fileManager(std::make_unique<FileManager>(m_configManager.get())),
    m_layoutParser(std::make_unique<LayoutParser>()),
    m_layoutBackend(std::make_unique<LayoutBackend>(*m_fileManager, *m_layoutParser)),
    m_defineManager(std::make_unique<DefineManager>()),
    m_defineBackend(std::make_unique<DefineBackend>()),
    m_flagManager(std::make_unique<FlagManager>(m_configManager.get())),
    m_textManager(std::make_unique<TextManager>()),
    m_textBackend(std::make_unique<TextBackend>()),
    m_layoutManager(std::make_unique<LayoutManager>(*m_layoutParser, *m_layoutBackend)),

    // --- neue Grafik-Komponenten ---
    m_themeManager(std::make_unique<ThemeManager>(m_fileManager.get())),
    m_behaviorManager(std::make_unique<BehaviorManager>(
        m_flagManager.get(),
        m_textManager.get(),
        m_defineManager.get(),
        m_layoutManager.get(),
        m_layoutBackend.get())),
    m_renderManager(std::make_unique<RenderManager>(
        m_themeManager.get(),
        m_behaviorManager.get()))
{
    connect(m_layoutManager.get(), &LayoutManager::tokensReady,
            this, &ProjectController::onTokensReady);
    qInfo() << "[ProjectController] Initialisiert";
}

void ProjectController::onTokensReady()
{
    qInfo() << "[ProjectController] TokensReady empfangen -> Rebuild aller Manager";

    // 1) Originaldaten vom TokenData geholt
    const auto tokenMap = TokenData::instance().all();   // QMap<QString, QList<Token>>

    m_tokensReady = true;

    // 2) Flache Liste für Manager, die QList<Token> erwarten
    QList<Token> flatTokens;
    for (const auto& list : tokenMap)
        flatTokens.append(list);

    // 3) LayoutManager übernimmt Tokens direkt aus dem Singleton
    {
        m_layoutManager->refreshFromParser();
        m_layoutManager->processLayout();
    }

    if (m_behaviorManager) {
        auto processedWindows = m_layoutManager->processedWindows();
        for (auto& wnd : processedWindows) {
            if (!wnd) continue;
            for (auto& ctrl : wnd->controls) {
                if (ctrl)
                    m_behaviorManager->applyBehavior(*ctrl);
            }
        }
    }

    // 4) DefineManager erwartet LISTE → flache Liste geben
    if (m_defineManager)
        m_defineManager->rebuildFromTokens(flatTokens);

    // 5) TextManager erwartet LISTE → flache Liste geben
    if (m_textManager)
        m_textManager->rebuildFromTokens(flatTokens);
    emit projectLoaded();

    qInfo() << "[ProjectController] Token-basierter Rebuild abgeschlossen.";
}

// --------------------------------------------------
// Projekt laden (mit automatischer Benutzerabfrage)
// --------------------------------------------------
bool ProjectController::loadProject(const QString& configPath)
{
    qInfo() << "[ProjectController] Starte Projekt-Ladevorgang...";
    m_loadingActive = true;
    m_tokensReady = false;

    const QString cfgFile = configPath.isEmpty()
                                ? ConfigManager::defaultConfigPath()
                                : configPath;

    if (!QFileInfo::exists(cfgFile)) {
        qInfo() << "[ProjectController] Keine Config vorhanden – Benutzer wird gefragt.";

        QString resdataFile = QFileDialog::getOpenFileName(
            nullptr, "Wähle deine Layout-Datei (resdata.inc)",
            QString(), "Layout-Dateien (*.inc *.txt);;Alle Dateien (*.*)");
        if (resdataFile.isEmpty()) return false;

        QString iconDir  = QFileDialog::getExistingDirectory(nullptr, "Wähle den Icon-Ordner");
        if (iconDir.isEmpty()) return false;
        QString themeDir = QFileDialog::getExistingDirectory(nullptr, "Wähle den Theme-Ordner");
        if (themeDir.isEmpty()) return false;

        QString sourceDir;
        while (sourceDir.isEmpty() || !QDir(sourceDir).exists()) {
            sourceDir = QFileDialog::getExistingDirectory(nullptr, "Wähle den Source-Ordner (Pflicht)");
            if (sourceDir.isEmpty()) {
                QMessageBox::warning(nullptr, "Pflichtfeld",
                                     "Der Source-Ordner ist erforderlich, um Flags zu erzeugen.");
            }
        }

        m_configManager->setLayoutPath(resdataFile);
        m_configManager->setIconPath(iconDir);
        m_configManager->setThemePath(themeDir);
        m_configManager->setSourcePath(sourceDir);
        m_configManager->save(cfgFile);
    }

    if (!m_configManager->load(cfgFile)) {
        qWarning() << "[ProjectController] Konnte Config nicht laden:" << cfgFile;
        return false;
    }

    const QString resdataFile = m_configManager->layoutPath();
    m_fileManager->cacheLayoutPath(resdataFile);
    const QString themeDir    = m_configManager->themePath();
    const QString iconDir     = m_configManager->iconPath();
    const QString sourceDir   = m_configManager->sourcePath();

    // -----------------------------------------
    // 2️⃣ Flags prüfen + ggf. neu generieren
    // -----------------------------------------
    const QString configDir = QFileInfo(cfgFile).absolutePath();
    const QString wndFlagsPath  = configDir + "/window_flags.json";
    const QString ctrlFlagsPath = configDir + "/control_flags.json";

    const bool flagsMissing =
        !QFileInfo::exists(wndFlagsPath) || !QFileInfo::exists(ctrlFlagsPath);

    if (flagsMissing) {
        qInfo() << "[ProjectController] Flags fehlen → Erstelle neu.";
        m_flagManager->generateFlags(sourceDir, wndFlagsPath, ctrlFlagsPath);
    }

    // -----------------------------------------
    // 3️⃣ LayoutManager vorbereiten
    // -----------------------------------------
    if (m_behaviorManager)
        m_behaviorManager->refreshFlagsFromFiles();

    // -----------------------------------------
    // 4️⃣ Layout laden (startet Parser & Tokens)
    // -----------------------------------------
    m_layoutBackend->setPath(resdataFile);
    m_layoutBackend->load();

    // Parser startet asynchron und triggert irgendwann onTokenReady()

    // -----------------------------------------
    // 5️⃣ LayoutManager verknüpfen & verarbeiten
    // -----------------------------------------
    m_layoutManager->refreshFromParser();
    m_layoutManager->processLayout();
    qInfo() << "[ProjectController] Layout-Daten vollständig initialisiert – sende layoutsReady()";
    emit layoutsReady();

    auto processedWindows = m_layoutManager->processedWindows();
    qInfo() << "[ProjectController] Processed Layouts:" << processedWindows.size();

    // -----------------------------------------
    // 6️⃣ Defines & Texte laden
    // -----------------------------------------
    const QString defineFile   = m_fileManager->findDefineFile(resdataFile);
    const QString textFile     = m_fileManager->findTextFile(resdataFile);
    const QString textIncFile  = m_fileManager->findTextIncFile(resdataFile);

    if (!defineFile.isEmpty())
        m_defineBackend->load(defineFile, *m_defineManager);

    if (!textFile.isEmpty())
        m_textBackend->loadText(textFile, *m_textManager);

    if (!textIncFile.isEmpty())
        m_textBackend->loadInc(textIncFile, *m_textManager);

    // -----------------------------------------
    // 7️⃣ Ressourcen (Icons / Themes)
    // -----------------------------------------
    m_icons = ResourceUtils::loadIcons(iconDir);

    // Theme laden über ThemeManager
    if (m_themeManager)
    {
        // Standardmäßig das Default-Theme laden
        QString defaultTheme = "Default";

        // Optional: wenn "English" oder anderes existiert
        QString englishPath = m_fileManager->themeFolderPath("English");
        if (QDir(englishPath).exists())
            defaultTheme = "English";

        qInfo().noquote() << "[ProjectController] Lade Theme über ThemeManager:" << defaultTheme;
        m_themeManager->loadTheme(defaultTheme);
    }
    else
    {
        qWarning() << "[ProjectController] Kein ThemeManager verfügbar!";
    }

    // -----------------------------------------
    // 🔒 Abschluss-Check: Tokens schon da?
    // -----------------------------------------
    if (m_tokensReady) {
        qInfo() << "[ProjectController] Tokens bereits verfügbar → Projekt wird jetzt als geladen markiert.";

        emit projectLoaded();

        // Fensterdaten asynchron verteilen
        QTimer::singleShot(0, this, [this, processedWindows]() {
            if (!processedWindows.empty() && processedWindows.front()) {
                emit windowsReady(processedWindows);
                emit activeWindowChanged(processedWindows.front());
            }
        });

        m_loadingActive = false;
    } else {
        qInfo() << "[ProjectController] Tokens fehlen noch → Warte auf onTokenReady().";
    }

    qInfo() << "[ProjectController] Themes:" << m_themes.size()
            << "Icons:" << m_icons.size();

    return true;
}

// --------------------------------------------------
// Projekt speichern
// --------------------------------------------------
bool ProjectController::saveProject()
{
    // ----------------------------------------------------------
    // Sicherheit: Prüfen, ob alles da ist
    // ----------------------------------------------------------
    if (!m_layoutManager || !m_layoutBackend) {
        qWarning() << "[ProjectController] Layout-Komponenten fehlen!";
        return false;
    }

    if (!m_defineManager || !m_defineBackend) {
        qWarning() << "[ProjectController] Define-Komponenten fehlen!";
        return false;
    }

    if (!m_textManager || !m_textBackend) {
        qWarning() << "[ProjectController] Text-Komponenten fehlen!";
        return false;
    }

    QString layoutPath = m_fileManager->layoutPath();
    if (layoutPath.isEmpty()) {
        qWarning() << "[ProjectController] Kein Layout-Pfad gesetzt!";
        return false;
    }

    QString definePath = m_fileManager->definePath();
    if (definePath.isEmpty()) {
        qWarning() << "[ProjectController] Kein Define-Pfad gesetzt!";
        return false;
    }

    // ----------------------------------------------------------
    // 1️⃣ Layout speichern
    // ----------------------------------------------------------
    QString layoutContent = m_layoutManager->serializeLayout();
    if (!m_layoutBackend->writeFile(layoutPath, layoutContent)) {
        qWarning() << "[ProjectController] Layout speichern fehlgeschlagen!";
        return false;
    }

    // ----------------------------------------------------------
    // 2️⃣ Defines speichern
    // ----------------------------------------------------------
    if (!m_defineBackend->saveDefines(definePath, *m_defineManager)) {
        qWarning() << "[ProjectController] Define-Datei speichern fehlgeschlagen!";
        return false;
    }

    // ----------------------------------------------------------
    // 3️⃣ Texte speichern (nur wenn Dirty)
    // ----------------------------------------------------------
    QString textPath;
    QString textIncPath;
    if (m_textManager->isDirty()) {
        textPath = m_fileManager->textPath();
        if (textPath.isEmpty()) {
            qWarning() << "[ProjectController] Kein Text-Pfad gefunden!";
            return false;
        }

        textIncPath = m_fileManager->textIncPath();
        if (textIncPath.isEmpty()) {
            qWarning() << "[ProjectController] Kein Text-INC-Pfad gefunden!";
            return false;
        }

        if (!m_textBackend->saveText(textPath, *m_textManager)) {
            qWarning() << "[ProjectController] Textdatei speichern fehlgeschlagen!";
            return false;
        }

        if (!m_textBackend->saveInc(textIncPath, *m_textManager)) {
            qWarning() << "[ProjectController] Text-INC-Datei speichern fehlgeschlagen!";
            return false;
        }

        m_textManager->clearDirty();
    } else {
        textPath    = m_fileManager->textPath();
        textIncPath = m_fileManager->textIncPath();
    }

    // ----------------------------------------------------------
    // Fertig!
    // ----------------------------------------------------------
    emit projectSaved();
    qInfo().noquote()
        << "[ProjectController] Projekt erfolgreich gespeichert:"
        << "\n   Layout :" << layoutPath
        << "\n   Defines:" << definePath
        << "\n   Texte  :" << (textPath.isEmpty() ? QStringLiteral("<unbekannt>") : textPath)
        << "\n   Text-INC:" << (textIncPath.isEmpty() ? QStringLiteral("<unbekannt>") : textIncPath);

    return true;
}

void ProjectController::selectWindow(const QString& windowName)
{
    if (!m_layoutManager)
        return;

    auto wnd = m_layoutManager->findWindow(windowName);
    if (!wnd)
        return;

    m_currentWindow  = wnd;
    m_currentControl = nullptr;     // wenn Fenster gewählt → Control zurücksetzen

    qInfo() << "[ProjectController] Window selected:" << windowName;

    emit selectionChanged();
    emit activeWindowChanged(wnd);
}

void ProjectController::selectControl(const QString& windowName, const QString& controlName)
{
    if (!m_layoutManager)
        return;

    auto wnd = m_layoutManager->findWindow(windowName);
    if (!wnd)
        return;

    std::shared_ptr<ControlData> foundCtrl;
    for (const auto& ctrl : wnd->controls)
    {
        if (ctrl && ctrl->id == controlName)
        {
            foundCtrl = ctrl;
            break;
        }
    }

    if (!foundCtrl)
        return;

    m_currentWindow  = wnd;
    m_currentControl = foundCtrl;

    qInfo() << "[ProjectController] Control selected:" << controlName << "in" << windowName;

    emit selectionChanged();
}

 void ProjectController::bindCanvas(CanvasHandler* handler)
 {
     connect(this, &ProjectController::activeWindowChanged,
             handler, &CanvasHandler::onActiveWindowChanged);
 }

void ProjectController::bindPanels(WindowPanel* windowPanel, PropertyPanel* propertyPanel)
{
    if (!windowPanel || !propertyPanel)
        return;

    qInfo() << "[ProjectController] Panels verbunden (entkoppelt).";

    // ----------------------------------------------------
    // 🔹 Verbindung: WindowPanel → Controller
    // ----------------------------------------------------
    connect(windowPanel, &WindowPanel::windowSelected,
            this, &ProjectController::selectWindow);

    connect(windowPanel, &WindowPanel::controlSelected,
            this, [this](const QString& wndName, const QString& ctrlName) {
                selectControl(wndName, ctrlName);
            });

    // ----------------------------------------------------
    // 🔹 Verbindung: Controller → PropertyPanel
    // ----------------------------------------------------
    connect(this, &ProjectController::selectionChanged,
            this, [this, propertyPanel]() {
                auto wnd = currentWindow();
                auto ctrl = currentControl();

                if (ctrl)
                    propertyPanel->showControlProps(wnd, ctrl);
                else if (wnd)
                    propertyPanel->showWindowProps(wnd);
                else
                    propertyPanel->clear();
            });

    // ----------------------------------------------------
    // 🔹 Verbindung: Controller → PropertyPanel Refresh
    // ----------------------------------------------------
    connect(this, &ProjectController::uiRefreshRequested, propertyPanel, [this, propertyPanel]() {
        if (!propertyPanel || propertyPanel->isHidden())
            return;

        auto wnd = currentWindow();
        auto ctrl = currentControl();

        if (!ctrl && !wnd)
            return;

        QSignalBlocker blocker(propertyPanel);

        if (ctrl)
            propertyPanel->showControlProps(wnd, ctrl);
        else
            propertyPanel->showWindowProps(wnd);
    });

    // ----------------------------------------------------
    // 🔹 Verbindung: PropertyPanel → Controller (Flags geändert)
    // ----------------------------------------------------
    connect(propertyPanel, &PropertyPanel::flagsChanged,
            this, [this](quint32 newMask) {
                auto wnd  = currentWindow();
                auto ctrl = currentControl();

                if (!m_behaviorManager) {
                    qWarning() << "[ProjectController] Kein BehaviorManager vorhanden – Flags ignoriert.";
                    return;
                }
                if (!wnd && !ctrl) {
                    qWarning() << "[ProjectController] Kein aktives Fenster oder Control beim Flag-Update.";
                    return;
                }

                const QMap<QString, quint32> flagMap =
                    (ctrl ? m_behaviorManager->controlFlags()
                          : m_behaviorManager->windowFlags());

                if (ctrl) {
                    ctrl->flagsMask = newMask;
                    ctrl->resolvedMask.clear();

                    for (auto it = flagMap.constBegin(); it != flagMap.constEnd(); ++it) {
                        if (ctrl->flagsMask & it.value())
                            ctrl->resolvedMask << it.key();
                    }

                    // Konsistent: BehaviorManager darf noch zusätzliche Dinge tun
                    m_behaviorManager->updateControlFlags(ctrl);

                    qInfo() << "[ProjectController] Control flags aktualisiert für" << ctrl->id;
                }
                else if (wnd) {
                    wnd->flagsMask = newMask;
                    wnd->resolvedMask.clear();

                    for (auto it = flagMap.constBegin(); it != flagMap.constEnd(); ++it) {
                        if (wnd->flagsMask & it.value())
                            wnd->resolvedMask << it.key();
                    }

                    m_behaviorManager->updateWindowFlags(wnd);

                    qInfo() << "[ProjectController] Window flags aktualisiert für" << wnd->name;
                }

                qInfo() << "[ProjectController] → UI-Refresh angefordert";
                emit uiRefreshRequested();
            });


    // ----------------------------------------------------
    // 🔹 Initialisierung nach Projekt-Ladevorgang
    // ----------------------------------------------------
    connect(this, &ProjectController::windowsReady,
            this, [this, propertyPanel](const std::vector<std::shared_ptr<WindowData>>& windows) {
                if (!windows.empty() && windows.front())
                    propertyPanel->showWindowProps(windows.front());
            });

    // ----------------------------------------------------
    // 🔹 Erster Refresh, falls bereits aktiv
    // ----------------------------------------------------
    if (auto wnd = currentWindow())
        propertyPanel->showWindowProps(wnd);
}


void ProjectController::toggleControlFlag(const QString& flagName, bool enabled)
{
    auto ctrl = currentControl();
    if (!ctrl || !m_behaviorManager)
        return;

    const auto& flagMap = m_behaviorManager->controlFlags();
    if (!flagMap.contains(flagName))
        return;

    quint32 bit = flagMap.value(flagName);

    if (enabled)
        ctrl->flagsMask |= bit;
    else
        ctrl->flagsMask &= ~bit;

    ctrl->resolvedMask.clear();
    for (auto it = flagMap.constBegin(); it != flagMap.constEnd(); ++it) {
        if (ctrl->flagsMask & it.value())
            ctrl->resolvedMask << it.key();
    }

    qInfo().noquote() << QString("[ProjectController] Control flags aktualisiert für \"%1\"")
                             .arg(ctrl->id);

    emit uiRefreshRequested();
}

void ProjectController::toggleWindowFlag(const QString& flag, bool enable)
{
    if (!m_currentWindow || !m_behaviorManager)
        return;

    const auto& map = m_behaviorManager->windowFlags();
    if (!map.contains(flag))
        return;

    quint32 bit = map[flag];
    if (enable)
        m_currentWindow->flagsMask |= bit;
    else
        m_currentWindow->flagsMask &= ~bit;

    m_behaviorManager->updateWindowFlags(m_currentWindow);

    emit uiRefreshRequested();
}

std::shared_ptr<WindowData> ProjectController::findWindow(const QString& name) const
{
    auto lm = layoutManager();
    if (!lm)
        return nullptr;

    // LayoutManager hat eigene findWindow()
    return lm->findWindow(name);
}

std::shared_ptr<ControlData> ProjectController::findControl(const QString& id) const
{
    auto wnd = currentWindow();
    if (!wnd)
        return nullptr;

    for (const auto& ctrl : wnd->controls) {
        if (ctrl && ctrl->id == id)
            return ctrl;
    }

    return nullptr;
}

void ProjectController::updateWindowFlags(const QString& windowName, quint32 mask, bool enabled)
{
    auto wnd = findWindow(windowName);
    if (!wnd) {
        qWarning().noquote() << "[ProjectController] updateWindowFlags(): Window nicht gefunden:" << windowName;
        return;
    }

    if (!m_behaviorManager) {
        qWarning().noquote() << "[ProjectController] updateWindowFlags(): Kein BehaviorManager verfügbar.";
        return;
    }

    // Flag-Namen aus BehaviorManager holen
    const auto& allFlags = m_behaviorManager->windowFlags();
    QString flagName;

    for (auto it = allFlags.constBegin(); it != allFlags.constEnd(); ++it) {
        if (it.value() == mask) {
            flagName = it.key();
            break;
        }
    }

    if (flagName.isEmpty()) {
        qWarning().noquote() << "[ProjectController] Unbekannter Flag-Mask:"
                             << QString("0x%1").arg(mask, 0, 16);
        return;
    }

    // Flag setzen oder entfernen
    if (enabled) {
        wnd->flagsMask |= mask;
        if (!wnd->resolvedMask.contains(flagName))
            wnd->resolvedMask.append(flagName);
    } else {
        wnd->flagsMask &= ~mask;
        wnd->resolvedMask.removeAll(flagName);
    }

    // BehaviorManager aktualisiert ggf. weitere abgeleitete Infos
    m_behaviorManager->updateWindowFlags(wnd);

    qInfo().noquote() << QString("[ProjectController] Window '%1' Flags aktualisiert → %2 (%3)")
                             .arg(windowName)
                             .arg(flagName)
                             .arg(enabled ? "ON" : "OFF");

    emit uiRefreshRequested();
}

void ProjectController::updateControlFlags(const QString& controlId, quint32 mask, bool enabled)
{
    auto ctrl = findControl(controlId);
    if (!ctrl) {
        qWarning().noquote() << "[ProjectController] updateControlFlags(): Control nicht gefunden:" << controlId;
        return;
    }

    if (!m_behaviorManager) {
        qWarning().noquote() << "[ProjectController] updateControlFlags(): Kein BehaviorManager verfügbar.";
        return;
    }

    const auto& allFlags = m_behaviorManager->controlFlags();
    QString flagName;

    for (auto it = allFlags.constBegin(); it != allFlags.constEnd(); ++it) {
        if (it.value() == mask) {
            flagName = it.key();
            break;
        }
    }

    if (flagName.isEmpty()) {
        qWarning().noquote() << "[ProjectController] Unbekannter Control-Flag-Mask:"
                             << QString("0x%1").arg(mask, 0, 16);
        return;
    }

    // Maske aktualisieren
    if (enabled)
        ctrl->flagsMask |= mask;
    else
        ctrl->flagsMask &= ~mask;

    // BehaviorManager baut resolvedMask neu auf / ergänzt
    m_behaviorManager->updateControlFlags(ctrl);

    qInfo().noquote() << QString("[ProjectController] Control '%1' Flags aktualisiert → %2 (%3)")
                             .arg(controlId)
                             .arg(flagName)
                             .arg(enabled ? "ON" : "OFF");

    emit uiRefreshRequested();
}
void ProjectController::requestUiRefreshAsync()
{
    QTimer::singleShot(0, this, [this]() {
        emit uiRefreshRequested();
    });
}
