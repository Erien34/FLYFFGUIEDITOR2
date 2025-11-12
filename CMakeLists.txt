cmake_minimum_required(VERSION 3.20)

project(FLYFFGUIEDITOR2 LANGUAGES CXX)

# ============================================================
# 🧱 1. QT & Compiler Setup
# ============================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# ============================================================
# 📂 2. Quellcode-Gruppen
# ============================================================

# ---- Core ----
set(SRC_CORE
    src/core/ProjectController.cpp
    src/core/ProjectController.h
    src/core/ConfigManager.cpp
    src/core/ConfigManager.h
    src/core/FileManager.cpp
    src/core/FileManager.h
)

# ---- Editor ----
set(SRC_EDITOR
    src/editor/Canvas.cpp
    src/editor/Canvas.h
    src/editor/CanvasHandler.cpp
    src/editor/CanvasHandler.h
)

# ---- Render ----
set(SRC_RENDER
    src/render/RenderManager.cpp
    src/render/RenderManager.h
    src/render/RenderWindow.cpp
    src/render/RenderWindow.h
    src/render/RenderControls.cpp
    src/render/RenderControls.h
    src/render/RenderMode.h
    src/render/ControlState.h
    src/render/WindowState.h
)

# ---- Theme ----
set(SRC_THEME
    src/theme/ThemeManager.cpp
    src/theme/ThemeManager.h
    src/theme/ThemeDefinition.h
)

# ---- Behavior ----
set(SRC_BEHAVIOR
    src/behavior/BehaviorManager.cpp
    src/behavior/BehaviorManager.h
)

# ---- Layout ----
set(SRC_LAYOUT
    src/layout/LayoutParser.cpp
    src/layout/LayoutParser.h
    src/layout/LayoutBackend.cpp
    src/layout/LayoutBackend.h
    src/layout/LayoutManager.cpp
    src/layout/LayoutManager.h
)

# ---- Layout Models ----
set(SRC_LAYOUT_MODEL
    src/layout/model/WindowData.h
    src/layout/model/ControlData.h
    src/layout/model/TokenData.h
)

# ---- Defines ----
set(SRC_DEFINE
    src/define/DefineManager.cpp
    src/define/DefineManager.h
    src/define/DefineBackend.cpp
    src/define/DefineBackend.h
    src/define/FlagManager.cpp
    src/define/FlagManager.h
)

# ---- Text ----
set(SRC_TEXT
    src/text/TextManager.cpp
    src/text/TextManager.h
    src/text/TextBackend.cpp
    src/text/TextBackend.h
)

# ---- UI ----
set(SRC_UI
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/WindowPanel.cpp
    src/ui/WindowPanel.h
    src/ui/PropertyPanel.cpp
    src/ui/PropertyPanel.h
)

# ---- Utils ----
set(SRC_UTILS
    src/utils/BaseManager.cpp
    src/utils/BaseManager.h
    src/utils/EncodingUtils.h
    src/utils/ResourceUtils.h
)

# ---- Main ----
set(SRC_MAIN
    src/main.cpp
)

# ============================================================
# 🧩 3. Zusammenführen aller Quellen
# ============================================================
set(SRC_FILES
    ${SRC_CORE}
    ${SRC_EDITOR}
    ${SRC_RENDER}
    ${SRC_THEME}
    ${SRC_BEHAVIOR}
    ${SRC_UTILS}
    ${SRC_LAYOUT}
    ${SRC_LAYOUT_MODEL}
    ${SRC_DEFINE}
    ${SRC_TEXT}
    ${SRC_UI}
    ${SRC_MAIN}
)

# ============================================================
# ⚙️ 4. Executable & Includes
# ============================================================
add_executable(${PROJECT_NAME} ${SRC_FILES}
    WindowState.h
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/core
    src/editor
    src/render
    src/theme
    src/behavior
    src/utils
    src/layout
    src/layout/model
    src/define
    src/text
    src/ui
)

# ============================================================
# 🔗 5. Qt Libraries
# ============================================================
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# ============================================================
# 🧭 6. IDE-Gruppierung (optional)
# ============================================================
source_group("Core" FILES ${SRC_CORE})
source_group("Editor" FILES ${SRC_EDITOR})
source_group("Render" FILES ${SRC_RENDER})
source_group("Theme" FILES ${SRC_THEME})
source_group("Behavior" FILES ${SRC_BEHAVIOR})
source_group("Layout" FILES ${SRC_LAYOUT})
source_group("Layout/Model" FILES ${SRC_LAYOUT_MODEL})
source_group("Defines" FILES ${SRC_DEFINE})
source_group("Text" FILES ${SRC_TEXT})
source_group("UI" FILES ${SRC_UI})
source_group("Utils" FILES ${SRC_UTILS})
source_group("Main" FILES ${SRC_MAIN})

# ============================================================
# ⚡ 7. Compiler Warnungen
# ============================================================
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ============================================================
# 🧹 8. Optionale Ordner ausschließen
# ============================================================
# add_subdirectory(src/dump)  # Alte Dateien ignorieren
