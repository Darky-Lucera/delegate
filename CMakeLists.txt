cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME delegate)
project(${PROJECT_NAME} VERSION 1.2.1 LANGUAGES CXX)

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
    src/Delegate.h
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Tests: run them with ctest
#--------------------------------------
# include(CTest) writes the DartConfiguration.tcl that "ctest -T test" needs (VS Code CMake Tools runs it that way).
# CTEST_TARGETS_ADDED stops it from adding the Continuous, Experimental and Nightly dashboard targets.
set_property(GLOBAL PROPERTY CTEST_TARGETS_ADDED 1)
include(CTest)

if(BUILD_TESTING)
    foreach(TEST_NAME DelegateTests DelegateAllocTests)
        add_executable(${TEST_NAME}
            tests/${TEST_NAME}.cpp
            tests/TestRunner.h
            src/Delegate.h
        )

        target_include_directories(${TEST_NAME}
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

        if(MSVC)
            target_compile_options(${TEST_NAME} PRIVATE /W4 /permissive-)
        else()
            target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic)
        endif()

        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()
