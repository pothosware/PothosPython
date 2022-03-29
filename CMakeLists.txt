########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.9)
project(PothosPython CXX)

if(${CMAKE_PROJECT_NAME} STREQUAL ${PROJECT_NAME})
    find_package(Pothos "0.6.0" CONFIG REQUIRED)
else()
    find_package(Pothos CONFIG REQUIRED) #in-tree build
endif()

list(INSERT CMAKE_MODULE_PATH 0 ${PROJECT_SOURCE_DIR}/cmake)
include(PothosPythonUtil)

#help find_package(PythonLibs) by setting Python_ADDITIONAL_VERSIONS from PYTHON_VERSION_STRING
if(PYTHONINTERP_FOUND AND DEFINED PYTHON_VERSION_STRING AND NOT DEFINED Python_ADDITIONAL_VERSIONS)
    string(SUBSTRING "${PYTHON_VERSION_STRING}" 0 3 Python_ADDITIONAL_VERSIONS)
endif()

########################################################################
## Find python
########################################################################
set(PYTHON_CONFIG_EXECUTABLE ${PYTHON_EXECUTABLE}-config
    CACHE FILEPATH "Path to python-config executable")
if (USE_PYTHON_CONFIG AND EXISTS ${PYTHON_CONFIG_EXECUTABLE})
    execute_process(
        COMMAND ${PYTHON_CONFIG_EXECUTABLE}  --includes
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE PYTHON_INCLUDE_DIRS)
    string(REGEX REPLACE "^[-I]" "" PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIRS}")
    string(REGEX REPLACE "[ ]-I" " " PYTHON_INCLUDE_DIRS "${PYTHON_INCLUDE_DIRS}")
    separate_arguments(PYTHON_INCLUDE_DIRS)

    #for python3.8 and above, use --embed to link python into the pothos module
    #pothos module util should fail if symbols are missing, but if this is bypassed
    #the runtime load of the python support module will fail without the --embed
    #earlier version of python config executable do not have this flag error with it
    if (PYTHON_VERSION_STRING AND "${PYTHON_VERSION_STRING}" GREATER_EQUAL "3.8")
        list(APPEND PYTHON_CONFIG_EXTRA_ARGS --embed)
    endif()

    execute_process(
        COMMAND ${PYTHON_CONFIG_EXECUTABLE}  --ldflags
        ${PYTHON_CONFIG_EXTRA_ARGS}
        OUTPUT_STRIP_TRAILING_WHITESPACE
        OUTPUT_VARIABLE PYTHON_LIBRARIES)
    set(PYTHONLIBS_VERSION_STRING ${PYTHON_VERSION_STRING})
    set(PYTHONLIBS_FOUND TRUE)
else()
    find_package(PythonLibs)
endif()

message(STATUS "PYTHONLIBS_FOUND: ${PYTHONLIBS_FOUND} - ${PYTHONLIBS_VERSION_STRING}")
message(STATUS "PYTHON_INCLUDE_DIRS: ${PYTHON_INCLUDE_DIRS}")
message(STATUS "PYTHON_LIBRARIES: ${PYTHON_LIBRARIES}")

# enable python debug mode when the debug library is specified
if (PYTHON_DEBUG_LIBRARY)
    add_definitions(-DPy_DEBUG)
    set(PYTHON_DEBUG_POSTFIX _d)
else()
    unset(PYTHON_DEBUG_POSTFIX)
endif()

#on windows, we require a pythonxx_d.lib in debug mode
#require that the PYTHON_DEBUG_LIBRARY flag is set
#or the build assumes that the debug library DNE
set(PYTHON_DEBUG_OK TRUE)
if(WIN32 AND NOT PYTHON_DEBUG_LIBRARY AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    message(WARNING "WIN32 Debug mode requires PYTHON_DEBUG_LIBRARY")
    set(PYTHON_DEBUG_OK FALSE)
endif()

########################################################################
# Python version check
########################################################################
set(PYTHON_VERSION_MATCH TRUE)
if (PYTHON_VERSION_STRING AND PYTHONLIBS_VERSION_STRING)
    if(NOT "${PYTHON_VERSION_STRING}" VERSION_EQUAL "${PYTHONLIBS_VERSION_STRING}")
        message(WARNING "Python interp and library version mismatch")
        set(PYTHON_VERSION_MATCH FALSE)
    endif()
endif()

########################################################################
# json.hpp header
########################################################################
find_path(JSON_HPP_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

if (NOT JSON_HPP_INCLUDE_DIR)
    message(WARNING "Pothos Python toolkit requires json.hpp, skipping...")
endif (NOT JSON_HPP_INCLUDE_DIR)

########################################################################
## Feature registration
########################################################################
include(FeatureSummary)
include(CMakeDependentOption)
cmake_dependent_option(ENABLE_PYTHON "Enable Pothos Python component" ON "Pothos_FOUND;PYTHONLIBS_FOUND;PYTHON_DEBUG_OK;POTHOS_PYTHON_DIR;JSON_HPP_INCLUDE_DIR" OFF)
add_feature_info(Python ENABLE_PYTHON "Python bindings for the Pothos framework")
if (NOT ENABLE_PYTHON)
    return()
endif()

########################################################################
## Python<->Pothos proxy wrapper
########################################################################
include_directories(${PYTHON_INCLUDE_DIRS})
include_directories(${JSON_HPP_INCLUDE_DIR})

#generate include with build configuration params
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/PythonSupport.in.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/PythonSupport.hpp
@ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(SOURCES
   PythonProxy.cpp
   PythonHandle.cpp
   PythonConvert.cpp
   TestPython.cpp
   TestPythonBlock.cpp
   PythonBlock.cpp
   ProxyHelpers.cpp
   PythonConfLoader.cpp
   PythonLogger.cpp
   FrameworkTypes.cpp
   PythonInfo.cpp
)

POTHOS_MODULE_UTIL(
    TARGET PythonSupport
    SOURCES ${SOURCES}
    LIBRARIES ${PYTHON_LIBRARIES}
    DESTINATION proxy/environment
)

#apple clang has extremely strict visibility with templates and typeinfo
#this fixes the typeid for the registration for the pyobject* converter
if (APPLE)
set_property(TARGET PythonSupport PROPERTY CXX_VISIBILITY_PRESET default)
endif (APPLE)

#cant fix compile warnings from Python.h itself
if (CMAKE_COMPILER_IS_GNUCXX)
target_compile_options(PythonSupport PRIVATE -Wno-missing-braces)
target_compile_options(PythonSupport PRIVATE -Wno-missing-field-initializers)
endif (CMAKE_COMPILER_IS_GNUCXX)

########################################################################
# Install cmake helper modules
########################################################################
install(
    FILES
        cmake/PothosPythonUtil.cmake
        cmake/PothosPythonBlockFactory.in.cpp
    DESTINATION ${POTHOS_CMAKE_DIRECTORY}
)

########################################################################
# Enter the subdirectory configuration
########################################################################
add_subdirectory(Pothos)
add_subdirectory(Blocks)
add_subdirectory(TestBlocks)
