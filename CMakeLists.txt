cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME sss-guis)
project(${PROJECT_NAME}
        LANGUAGES CXX
        VERSION 0.8.2
        DESCRIPTION "An extensible YAML to graphical web application system")

include(GNUInstallDirs)

set(EXECUTABLE_NAME ${PROJECT_NAME}_executable)
set(LIBRARY_NAME ${PROJECT_NAME}_library)

set(CMAKE_INSTALL_RPATH "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)

find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

if (NOT TARGET yaml-cpp::yaml-cpp AND TARGET yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
endif()

add_subdirectory(js EXCLUDE_FROM_ALL)

set(SSS_GUIS_NPM_TARBALL ${SSS-GUIS-NPM-TARBALL} CACHE INTERNAL "")

set(SOURCES src/dependencies.cpp
            src/generation.cpp
            src/guis.cpp
            src/structure.cpp)

option(SSS_GUIS_BUILD_AS_SHARED_LIBRARY "Build as shared library" OFF)
option(SSS_GUIS_BUILD_AS_STATIC_LIBRARY "Build as static library" OFF)
option(SSS_GUIS_BUILD_EXECUTABLE "Build an executable" ON)

if (SSS_GUIS_BUILD_EXECUTABLE)
    if (NOT(SSS_GUIS_BUILD_AS_SHARED_LIBRARY OR SSS_GUIS_BUILD_AS_STATIC_LIBRARY))
        set(SSS_GUIS_BUILD_AS_SHARED_LIBRARY ON)
        message("-- ${PROJECT_NAME}: No library type was defined so defaulting to a shared library")
    endif()
endif()

if (SSS_GUIS_BUILD_AS_SHARED_LIBRARY)
    message("-- ${PROJECT_NAME}: Building ${PROJECT_NAME} as a shared library")
    add_library(${LIBRARY_NAME} SHARED ${SOURCES})
elseif (SSS_GUIS_BUILD_AS_STATIC_LIBRARY)
    message("-- ${PROJECT_NAME}: Building ${PROJECT_NAME} as a static library")
    add_library(${LIBRARY_NAME} STATIC ${SOURCES})
elseif (NOT(SSS_GUIS_BUILD_EXECUTABLE))
    message(FATAL_ERROR "${PROJECT_NAME}: Nothing is configured to be built")
endif()

set_target_properties(${LIBRARY_NAME} PROPERTIES
                      OUTPUT_NAME ${PROJECT_NAME}
                      EXPORT_NAME GUIS
)

target_include_directories(${LIBRARY_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${LIBRARY_NAME} PRIVATE Threads::Threads
                                              yaml-cpp::yaml-cpp
                                              nlohmann_json::nlohmann_json
                                              $<BUILD_INTERFACE:sss-guis-js>)

target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)
target_compile_options(${LIBRARY_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror -O3)

if (SSS_GUIS_BUILD_EXECUTABLE)
    message("   Building ${PROJECT_NAME} as an executable (as well)")
    add_executable(${EXECUTABLE_NAME} main.cpp)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME})
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
                               SSS_GUIS_VERSION_MAJOR=${${PROJECT_NAME}_VERSION_MAJOR}
                               SSS_GUIS_VERSION_MINOR=${${PROJECT_NAME}_VERSION_MINOR}
                               SSS_GUIS_VERSION_PATCH=${${PROJECT_NAME}_VERSION_PATCH}
    )
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

    install(TARGETS ${EXECUTABLE_NAME}
            DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(TARGETS ${LIBRARY_NAME}
        EXPORT SSS_GUIS_TARGETS
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sss/guis/
)

install(FILES ${SSS_GUIS_NPM_TARBALL}
        DESTINATION ${CMAKE_INSTALL_DATADIR}/sss/guis/
)

install(EXPORT SSS_GUIS_TARGETS
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE SSS::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_PACKAGE_VENDOR "SSS")
set(CPACK_PACKAGE_CONTACT "Bradley Marshall")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
include(CPack)
