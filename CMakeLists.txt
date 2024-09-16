cmake_minimum_required(VERSION 3.8)
project(sim_logs)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MACOSX_RPATH 1)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(MSVC)
    # fix C1128: number of sections exceeded object file format limit
    add_compile_options(/bigobj)
endif()

if(WIN32)
    # fix LNK1104: cannot open file 'libboost_regex-vc142-mt-x64-1_74.lib'
    set(Boost_USE_STATIC_LIBS ON)
endif()


# find dependencies
find_package(ament_cmake REQUIRED)

if(NOT COPPELIASIM_INCLUDE_DIR)
    if(DEFINED ENV{COPPELIASIM_ROOT_DIR})
        set(COPPELIASIM_INCLUDE_DIR $ENV{COPPELIASIM_ROOT_DIR}/programming/include)
    else()
        message(FATAL_ERROR "Environment variable COPPELIASIM_ROOT_DIR is not set")
    endif()
endif()
list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
    ${COPPELIASIM_INCLUDE_DIR}/cmake)
find_package(CoppeliaSim 4.5.1.7 REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Boost REQUIRED)

coppeliasim_add_plugin(
    simLogs
    LEGACY
    SOURCES
    src/sim_logs.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simLib/scriptFunctionData.cpp
    ${COPPELIASIM_INCLUDE_DIR}/simLib/scriptFunctionDataItem.cpp
)
coppeliasim_add_lua(lua/simLogs.lua)
coppeliasim_add_lua(lua/simLogs-ce.lua)

ament_package()
