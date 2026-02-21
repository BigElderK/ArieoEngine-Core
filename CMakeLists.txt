cmake_minimum_required (VERSION 3.31)

include($ENV{ARIEO_BUILD_ENV_PACKAGES_OUTPUT_DIR}/cmake/build_environment.cmake)

ArieoPackage(
    Arieo-Core
    CATEGORY 02_engine
    COMPONENTS
        arieo_core
)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    arieo_engine_project(
        arieo_core
        PROJECT_TYPE static_library

        DEPENDENCIES
            THIRDPARTY_PACKAGES
                easyloggingpp
                yaml-cpp
                fmt
                mathfu
                concurrentqueue
            PUBLIC_LIBS
                yaml-cpp::yaml-cpp
                fmt::fmt
                mathfu::mathfu
                concurrentqueue::concurrentqueue
            PRIVATE_LIBS
                log
                android
                easyloggingpp::easyloggingpp

        SOURCES
            PUBLIC_INCLUDE_FOLDERS
                ${CMAKE_CURRENT_LIST_DIR}/public/include
            
            CXX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*/*/*.cpp
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*/*.cpp
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*.cpp
    )
else()
    arieo_engine_project(
        arieo_core
        PROJECT_TYPE static_library

        DEPENDENCIES
            THIRDPARTY_PACKAGES
                easyloggingpp
                yaml-cpp
                fmt
                mathfu
                concurrentqueue
            PUBLIC_LIBS
                yaml-cpp::yaml-cpp
                fmt::fmt
                mathfu::mathfu
                concurrentqueue::concurrentqueue
            PRIVATE_LIBS
                easyloggingpp::easyloggingpp
        
        SOURCES
            PUBLIC_INCLUDE_FOLDERS
                ${CMAKE_CURRENT_LIST_DIR}/public/include
            
            CXX_SOURCES
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*/*/*.cpp
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*/*.cpp
                ${CMAKE_CURRENT_LIST_DIR}/private/src/*.cpp
    )
endif()
