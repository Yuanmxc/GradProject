cmake_minimum_required(VERSION 3.10.0)

project(dynlib_monitor_gui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Charts REQUIRED)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/eventdata.cpp
    src/timelineview.cpp
    src/processmanager.cpp
)

set(HEADERS
    src/mainwindow.h
    src/eventdata.h
    src/timelineview.h
    src/processmanager.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Charts
) 