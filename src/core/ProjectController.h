#pragma once

#include <QObject>
#include <memory>
#include <QMap>
#include <QIcon>
#include <QPixmap>
#include <QString>
#include <QDebug>

#include "CanvasHandler.h"
#include "layout/model/WindowData.h"
#include "ConfigManager.h"
#include "FileManager.h"
#include "layout/LayoutParser.h"
#include "layout/LayoutBackend.h"
#include "define/DefineManager.h"
#include "define/DefineBackend.h"
#include "define/FlagManager.h"
#include "text/TextManager.h"
#include "text/TextBackend.h"
#include "layout/LayoutManager.h"
#include "render/RenderManager.h"
#include "theme/ThemeManager.h"
#include "behavior/BehaviorManager.h"

class ProjectController : public QObject
{
    Q_OBJECT

public:
    explicit ProjectController(QObject* parent = nullptr);

    void bindCanvas(CanvasHandler* handler);
    void bindPanels(class WindowPanel* windowPanel, class PropertyPanel* propertyPanel);

    bool loadProject(const QString& configPath);
    bool saveProject();

    const QMap<QString, QIcon>& icons() const { return m_icons; }
    const QMap<QString, QPixmap>& themes() const { return m_themes; }

    LayoutManager* layoutManager() const { return m_layoutManager.get(); }
    TextManager* textManager() const { return m_textManager.get(); }
    BehaviorManager* behaviorManager() const { return m_behaviorManager.get(); }
    RenderManager* renderManager() const { return m_renderManager.get(); }
    ThemeManager* themeManager() const { return m_themeManager.get(); }

    std::shared_ptr<WindowData>  currentWindow() const { return m_currentWindow; }
    std::shared_ptr<ControlData> currentControl() const { return m_currentControl; }

    void toggleControlFlag(const QString& flag, bool enable);
    void toggleWindowFlag(const QString& flag, bool enable);
    void updateWindowFlags(const QString& windowName, quint32 mask, bool enabled);
    void updateControlFlags(const QString& controlId, quint32 mask, bool enabled);

    void requestUiRefreshAsync();

signals:
    void projectLoaded();
    void projectSaved();
    void layoutsReady();
    void activeWindowChanged(const std::shared_ptr<WindowData>& wnd);
    void windowsReady(const std::vector<std::shared_ptr<WindowData>>& windows);

    void selectionChanged();           // irgendwas wurde ausgewählt → Panels sollen neu rendern
    void uiRefreshRequested();         // z.B. nach Flags-Änderung

public slots:

    void selectWindow(const QString& windowName);
    void selectControl(const QString& windowName, const QString& controlName);

    // Canvas oder PropertyPanel kann sagen: „bitte alles neu“
    void requestUiRefresh() { emit uiRefreshRequested(); }

private slots:
    void onTokensReady();

private:
    // 🔧 Manager
    std::unique_ptr<ConfigManager>  m_configManager;
    std::unique_ptr<FileManager>    m_fileManager;
    std::unique_ptr<LayoutParser>   m_layoutParser;
    std::unique_ptr<LayoutBackend>  m_layoutBackend;
    std::unique_ptr<DefineManager>  m_defineManager;
    std::unique_ptr<DefineBackend>  m_defineBackend;
    std::unique_ptr<FlagManager>    m_flagManager;
    std::unique_ptr<TextManager>    m_textManager;
    std::unique_ptr<TextBackend>    m_textBackend;
    std::unique_ptr<LayoutManager>  m_layoutManager;
    std::unique_ptr<BehaviorManager> m_behaviorManager;
    std::unique_ptr<RenderManager> m_renderManager;
    std::unique_ptr<ThemeManager>  m_themeManager;

    // 🔧 Ressourcen
    QMap<QString, QIcon>   m_icons;
    QMap<QString, QPixmap> m_themes;

    std::shared_ptr<WindowData>  m_currentWindow;
    std::shared_ptr<ControlData> m_currentControl;
    std::shared_ptr<WindowData> findWindow(const QString& name) const;
    std::shared_ptr<ControlData> findControl(const QString& id) const;

    bool m_loadingActive = false;
    bool m_tokensReady = false;
};

