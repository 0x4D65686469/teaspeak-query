cmake_minimum_required(VERSION 3.15)
project(teaspeak_query VERSION 1.0.0 LANGUAGES CXX)

option(TQ_BUILD_SHARED    "Build shared library"  OFF)
option(TQ_BUILD_EXAMPLES  "Build examples"        ON)
option(TQ_BUILD_TESTS     "Build tests"           OFF)
option(TQ_USE_SANITIZERS  "Enable sanitizers"     OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(TQ_USE_SANITIZERS AND NOT MSVC)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

set(TQ_SOURCES
    src/escape.cpp
    src/params.cpp
    src/parser.cpp
    src/socket.cpp
    src/types.cpp
    src/client.cpp
)

set(TQ_PUBLIC_HEADERS
    include/teaspeak_query/teaspeak_query.hpp
    include/teaspeak_query/escape.hpp
    include/teaspeak_query/exceptions.hpp
    include/teaspeak_query/types.hpp
    include/teaspeak_query/params.hpp
    include/teaspeak_query/client.hpp
)

if(TQ_BUILD_SHARED)
    add_library(teaspeak_query SHARED ${TQ_SOURCES})
    target_compile_definitions(teaspeak_query PRIVATE TQ_BUILDING_DLL)
    target_compile_definitions(teaspeak_query PUBLIC  TQ_SHARED)
else()
    add_library(teaspeak_query STATIC ${TQ_SOURCES})
endif()

target_include_directories(teaspeak_query
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(teaspeak_query PUBLIC cxx_std_17)

find_package(Threads REQUIRED)
target_link_libraries(teaspeak_query PUBLIC Threads::Threads)

if(WIN32)
    target_link_libraries(teaspeak_query PRIVATE ws2_32)
    target_compile_definitions(teaspeak_query PRIVATE
        WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601)
endif()

if(TQ_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS teaspeak_query
    EXPORT teaspeak_queryTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/teaspeak_queryConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/teaspeak_queryConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/teaspeak_queryConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teaspeak_query
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/teaspeak_queryConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/teaspeak_queryConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teaspeak_query
)
install(EXPORT teaspeak_queryTargets
    FILE teaspeak_queryTargets.cmake
    NAMESPACE teaspeak_query::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/teaspeak_query
)