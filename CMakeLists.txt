cmake_minimum_required(VERSION 3.16)
project(reim-pass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(REIM_BUILD_TUI "Build reim-pass (ncurses TUI)" ON)
option(REIM_BUILD_GUI "Build reim-pass-gui (Qt Quick / QML GUI)" ON)

if(NOT REIM_BUILD_TUI AND NOT REIM_BUILD_GUI)
    message(FATAL_ERROR "Enable at least one of REIM_BUILD_TUI / REIM_BUILD_GUI")
endif()

#
# Common dependencies & core
#
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED IMPORTED_TARGET libsodium)
pkg_check_modules(SQLITE REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(JSON   REQUIRED IMPORTED_TARGET nlohmann_json)

add_library(reim-core STATIC
    src/core/merge.cpp
    src/core/config.cpp
)

target_include_directories(reim-core PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(reim-core PUBLIC
    PkgConfig::SODIUM
    PkgConfig::SQLITE
    PkgConfig::JSON
)

target_compile_options(reim-core PRIVATE -Wall -Wextra)

include(GNUInstallDirs)

#
# TUI: reim-pass
#
if(REIM_BUILD_TUI)
    pkg_check_modules(NCURSES REQUIRED IMPORTED_TARGET ncurses)

    add_executable(reim-pass
        src/tui/start.cpp
        src/tui/manager.cpp
    )

    target_link_libraries(reim-pass PRIVATE
        reim-core
        PkgConfig::NCURSES
    )

    target_compile_options(reim-pass PRIVATE -Wall -Wextra)

    install(TARGETS reim-pass RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

#
# GUI: reim-pass-gui
#
if(REIM_BUILD_GUI)
    find_package(Qt6 REQUIRED COMPONENTS Quick)
    qt_standard_project_setup(REQUIRES 6.5)

    qt_add_executable(reim-pass-gui
        src/gui/start.cpp
    )

    qt_add_qml_module(reim-pass-gui
        URI ReimPass
        VERSION 1.0
        QML_FILES
            qml/StartWindow.qml
            qml/ManagerWindow.qml
            qml/SidebarButton.qml
        SOURCES
            src/gui/qml/login.hpp
            src/gui/qml/login.cpp
    )

    target_include_directories(reim-pass-gui PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src/gui/qml
    )

    target_link_libraries(reim-pass-gui PRIVATE
        reim-core
        Qt6::Quick
    )

    target_compile_options(reim-pass-gui PRIVATE -Wall -Wextra)

    install(TARGETS reim-pass-gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()