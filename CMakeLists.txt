cmake_minimum_required(VERSION 3.22)
project(FlyffGuiEditor LANGUAGES CXX)

# -----------------------------------------
# 🔧 Compiler / Qt-Basiskonfiguration
# -----------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.9 REQUIRED COMPONENTS Core Gui Widgets)

# -----------------------------------------
# 🧩 Projektstruktur
# -----------------------------------------
set(SRC_CORE
    src/core/ProjectController.cpp
    src/core/ProjectController.h
    src/core/ConfigManager.cpp
    src/core/ConfigManager.h
    src/core/FileManager.cpp
    src/core/FileManager.h
)

set(SRC_LAYOUT
    src/layout/LayoutBackend.cpp
    src/layout/LayoutBackend.h
    src/layout/LayoutManager.cpp
    src/layout/LayoutManager.h
    src/layout/LayoutParser.cpp
    src/layout/LayoutParser.h
)

set(SRC_DEFINES
    src/defines/DefineBackend.cpp
    src/defines/DefineBackend.h
    src/defines/DefineManager.cpp
    src/defines/DefineManager.h
    src/defines/FlagManager.cpp
    src/defines/FlagManager.h
)

set(SRC_TEXTS
    src/texts/TextBackend.cpp
    src/texts/TextBackend.h
    src/texts/TextManager.cpp
    src/texts/TextManager.h
)

set(SRC_EDITOR
    src/editor/Canvas.cpp
    src/editor/Canvas.h
    src/editor/CanvasHandler.cpp
    src/editor/CanvasHandler.h
)

set(SRC_UI
    src/ui/MainWindow.cpp
    src/ui/MainWindow.h
    src/ui/WindowPanel.cpp
    src/ui/WindowPanel.h
    src/ui/PropertyPanel.cpp
    src/ui/PropertyPanel.h
)

set(SRC_UTILS
    src/utils/BaseManager.cpp
    src/utils/BaseManager.h
    src/utils/EncodingUtils.h
    src/utils/ResourceUtils.h
)

set(SRC_MAIN src/main.cpp)

# -----------------------------------------
# 🔗 Alle Dateien zusammenfassen
# -----------------------------------------
set(SRC_FILES
    ${SRC_CORE}
    ${SRC_LAYOUT}
    ${SRC_DEFINES}
    ${SRC_TEXTS}
    ${SRC_EDITOR}
    ${SRC_UI}
    ${SRC_UTILS}
    ${SRC_MAIN}
)

# -----------------------------------------
# 📦 Executable erzeugen
# -----------------------------------------
add_executable(${PROJECT_NAME} ${SRC_FILES}

    src/utils/RenderPreview.h
    src/layout/model/ControlData.h src/layout/model/TokenData.h src/layout/model/WindowData.h
    src/renderer/RenderControls.cpp src/renderer/RenderControls.h src/renderer/RenderWindow.cpp src/renderer/RenderWindow.h
    src/defines/BehaviorManager.h src/defines/BehaviorManager.cpp)

# -----------------------------------------
# 🔗 Mit Qt-Bibliotheken verknüpfen
# -----------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# -----------------------------------------
# 🧭 Includepfade
# -----------------------------------------
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/core
    src/layout
    src/defines
    src/texts
    src/editor
    src/ui
    src/utils
)

# -----------------------------------------
# 🧱 Windows-spezifische Libraries
# -----------------------------------------
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        user32.lib
        gdi32.lib
        shell32.lib
        ole32.lib
        uuid.lib
        comdlg32.lib
        advapi32.lib
    )
endif()
