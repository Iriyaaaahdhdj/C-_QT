cmake_minimum_required(VERSION 3.21)

project(EmotionStarJournal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(EmotionStarJournal
    src/main.cpp
    src/emotionentry.h
    src/journalrepository.h
    src/journalrepository.cpp
    src/starmapview.h
    src/starmapview.cpp
    src/breathingskydialog.h
    src/breathingskydialog.cpp
    src/mainwindow.h
    src/mainwindow.cpp
)

target_link_libraries(EmotionStarJournal PRIVATE Qt6::Core Qt6::Widgets)
