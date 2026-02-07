cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME sss-guis)

project(${PROJECT_NAME} LANGUAGES CXX VERSION 0.6.4)

set(EXECUTABLE_NAME ${PROJECT_NAME}_executable)
set(LIBRARY_NAME ${PROJECT_NAME}_library)

set(CMAKE_INSTALL_RPATH "")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
set(CMAKE_SKIP_RPATH FALSE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)

find_package(yaml-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(js EXCLUDE_FROM_ALL)

set(SSS_GUIS_DECLARATION_HEADER ${SSS-GUIS-DECLARATION-HEADER} CACHE INTERNAL "")

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
    install(TARGETS ${LIBRARY_NAME}
            DESTINATION lib
            PERMISSIONS
                OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
    )
elseif (SSS_GUIS_BUILD_AS_STATIC_LIBRARY)
    message("-- ${PROJECT_NAME}: Building ${PROJECT_NAME} as a static library")
    add_library(${LIBRARY_NAME} STATIC ${SOURCES})
elseif (NOT(SSS_GUIS_BUILD_EXECUTABLE))
    message(FATAL_ERROR "${PROJECT_NAME}: Nothing is configured to be built")
endif()

target_include_directories(${LIBRARY_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                                           PRIVATE ${SSS-GUIS-JS-HEADER})

target_link_libraries(${LIBRARY_NAME} PRIVATE Threads::Threads
                                              yaml-cpp::yaml-cpp
                                              nlohmann_json::nlohmann_json
                                              sss-guis-js)

target_compile_features(${LIBRARY_NAME} PUBLIC cxx_std_17)

target_compile_options(${LIBRARY_NAME} PRIVATE -Wall
                                               -Wextra
                                               -Wpedantic
                                               -Werror
                                               -Wold-style-cast
                                               -s
                                               -O3)

set_target_properties(${LIBRARY_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

if (SSS_GUIS_BUILD_EXECUTABLE)
    message("   Building ${PROJECT_NAME} as an executable (as well)")
    add_executable(${EXECUTABLE_NAME} main.cpp)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE ${LIBRARY_NAME})
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
                               SSS_GUIS_VERSION_MAJOR=${sss-guis_VERSION_MAJOR}
                               SSS_GUIS_VERSION_MINOR=${sss-guis_VERSION_MINOR}
                               SSS_GUIS_VERSION_PATCH=${sss-guis_VERSION_PATCH}
    )
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

    install(TARGETS ${EXECUTABLE_NAME}
            DESTINATION bin
            PERMISSIONS
                OWNER_READ OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE
    )
endif()

install(FILES ${SSS-GUIS-DECLARATION-HEADER}
        DESTINATION "/include/sss/guis/"
        PERMISSIONS
            OWNER_READ OWNER_EXECUTE
            GROUP_READ GROUP_EXECUTE
            WORLD_READ WORLD_EXECUTE
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
        DESTINATION "/usr/include/sss/guis"
        FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE
            GROUP_READ
            WORLD_READ
        DIRECTORY_PERMISSIONS
            OWNER_READ OWNER_WRITE
            GROUP_READ
            WORLD_READ
)
