cmake_minimum_required(VERSION 3.16)

project(helmet_detection LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOTEST ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS
    Core
    Gui
    Widgets
    REQUIRED
)

add_executable(helmet_detection
    main.cpp
    mainwindow.h
    mainwindow.cpp
    Resources/models/best.pt Resources/python/detect.py Resources/python/get_first_frame.py Resources/python/process_camera.py Resources/python/process_video.py
)

target_link_libraries(helmet_detection
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)
