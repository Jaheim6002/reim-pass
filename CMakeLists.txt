cmake_minimum_required(VERSION 3.16)
project(reim-pass LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM  REQUIRED IMPORTED_TARGET libsodium)
pkg_check_modules(SQLITE  REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(NCURSES REQUIRED IMPORTED_TARGET ncurses)
pkg_check_modules(JSON    REQUIRED IMPORTED_TARGET nlohmann_json)

add_executable(reim-pass
    src/start.cpp
    src/manager.cpp
    src/merge.cpp
    src/config.cpp
)

target_include_directories(reim-pass PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(reim-pass PRIVATE
    PkgConfig::SODIUM
    PkgConfig::SQLITE
    PkgConfig::NCURSES
    PkgConfig::JSON
)

target_compile_options(reim-pass PRIVATE -Wall -Wextra)

include(GNUInstallDirs)
install(TARGETS reim-pass RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})