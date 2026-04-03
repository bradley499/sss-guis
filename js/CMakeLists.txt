cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME sss-guis-js)

project(${PROJECT_NAME} LANGUAGES CXX)

set(WORKING_DIRECTORY_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/js-gen)
file(MAKE_DIRECTORY "${WORKING_DIRECTORY_LOCATION}")
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(PACKAGE_JSON_SOURCE "${SOURCE_DIR}/package.json")
set(PACKAGE_JSON_DESTINATION "${WORKING_DIRECTORY_LOCATION}/package.json")

configure_file(
    "${PACKAGE_JSON_SOURCE}"
    "${WORKING_DIRECTORY_LOCATION}/package.json"
    COPYONLY
)

add_custom_target(
    sss-guis_js_dependencies
    DEPENDS "${PACKAGE_JSON_DESTINATION}"
)

message("-- sss-guis: Installing npm dependencies")
execute_process(COMMAND npm install --no-engine-strict --loglevel=error --no-audit --no-fund
                WORKING_DIRECTORY "${WORKING_DIRECTORY_LOCATION}"
                RESULT_VARIABLE npm_install_result
                OUTPUT_VARIABLE npm_install_output
)

if(npm_install_result AND NOT npm_install_result EQUAL 0)
    message(FATAL_ERROR "Failed to install npm packages")
endif()

set(GENERATED_JS "${WORKING_DIRECTORY_LOCATION}/dist/guis.js")
set(GENERATED_DECLARATION "${WORKING_DIRECTORY_LOCATION}/dist/guis.d.ts")
set(GENERATED_INCLUDE_DIRECTORY "${WORKING_DIRECTORY_LOCATION}/dist/include")
set(GENERATED_HEADER "${GENERATED_INCLUDE_DIRECTORY}/guis.js.hpp")
set(TEMP_HEADER "${CMAKE_CURRENT_BINARY_DIR}/guis.js.hpp.temp")

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    "${GENERATED_JS}"
    "${GENERATED_DECLARATION}"
    "${GENERATED_INCLUDE_DIRECTORY}"
)

add_custom_target(
    sss-guis_js_bundle
    ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}" "${WORKING_DIRECTORY_LOCATION}"
    COMMAND npm run build
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_INCLUDE_DIRECTORY}/"
    COMMAND bash -c "set -e; set -o pipefail; echo 'unsigned char sss_guis_js[] = {' > ${TEMP_HEADER}; od -v -t x1 -An ${GENERATED_JS} | sed 's/[[:space:]]*\\([0-9a-f]\\{2\\}\\)/0x\\1, /g' >> ${TEMP_HEADER}; sed -i '$ s/, $/};/' ${TEMP_HEADER}; echo -e '\\nunsigned int sss_guis_js_len = sizeof(sss_guis_js);' >> ${TEMP_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TEMP_HEADER}" "${GENERATED_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E remove "${TEMP_HEADER}"
    WORKING_DIRECTORY "${WORKING_DIRECTORY_LOCATION}"
    COMMENT "Generating sss-guis JavaScript bundle"
    VERBATIM
)

set_property(SOURCE ${GENERATED_HEADER} PROPERTY GENERATED TRUE)
set_property(SOURCE ${GENERATED_DECLARATION} PROPERTY GENERATED TRUE)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE "${GENERATED_INCLUDE_DIRECTORY}")

set(SSS-GUIS-JS-HEADER ${GENERATED_INCLUDE_DIRECTORY} PARENT_SCOPE)
set(SSS-GUIS-DECLARATION-HEADER ${GENERATED_DECLARATION} PARENT_SCOPE)

add_dependencies(${PROJECT_NAME} sss-guis_js_dependencies sss-guis_js_bundle)
