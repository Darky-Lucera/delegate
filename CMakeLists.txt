cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME delegate)
project(${PROJECT_NAME} VERSION 1.0.0 LANGUAGES CXX)

# Set features
#--------------------------------------
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#--------------------------------------
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#--------------------------------------
add_executable(${PROJECT_NAME}
    main.cpp
    src/delegate.h
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)