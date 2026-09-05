cmake_minimum_required(VERSION 3.14)

set(PROJECT_NAME sss-guis-js)
project(${PROJECT_NAME}
        LANGUAGES NONE
        VERSION ${CMAKE_PROJECT_VERSION})

# Paths
set(WORKING_DIRECTORY_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/js-gen)
set(DIST_DIR "${WORKING_DIRECTORY_LOCATION}/dist")
set(GENERATED_PACKAGE_JSON "${DIST_DIR}/package.json")
set(GENERATED_JS "${DIST_DIR}/guis.js")
set(GENERATED_DECLARATION "${DIST_DIR}/guis.d.ts")
set(GENERATED_INCLUDE_DIRECTORY "${DIST_DIR}/include")
set(GENERATED_HEADER "${GENERATED_INCLUDE_DIRECTORY}/guis.js.hpp")
set(GENERATED_NPM_TARBALL_NAME "sss-guis-${CMAKE_PROJECT_VERSION}.tgz")
set(GENERATED_TARBALL "${WORKING_DIRECTORY_LOCATION}/${GENERATED_NPM_TARBALL_NAME}")
set(TEMP_HEADER "${CMAKE_CURRENT_BINARY_DIR}/guis.js.hpp.temp")

file(MAKE_DIRECTORY "${WORKING_DIRECTORY_LOCATION}")
file(MAKE_DIRECTORY "${DIST_DIR}")

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/" DESTINATION "${WORKING_DIRECTORY_LOCATION}")

message("-- sss-guis: Installing npm dependencies")
execute_process(
    COMMAND npm install --no-engine-strict --loglevel=notice --no-audit --no-fund
    WORKING_DIRECTORY "${WORKING_DIRECTORY_LOCATION}"
    RESULT_VARIABLE npm_install_result
)

if(npm_install_result AND NOT npm_install_result EQUAL 0)
    message(FATAL_ERROR "Failed to install npm packages")
endif()

# 1. Generate the correct package.json into a temporary safe space at configure time
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/package.json.tmp"
"{
    \"name\": \"@sss/sss-guis\",
    \"version\": \"${CMAKE_PROJECT_VERSION}\",
    \"description\": \"TypeScript declaration files and stubs for creating compatible SSS widgets\",
    \"type\": \"module\",
    \"main\": \"./widget.mjs\",
    \"types\": \"./widget.d.ts\",
    \"exports\": {
        \".\": { \"types\": \"./widget.d.ts\", \"import\": \"./widget.mjs\" },
        \"./widget\": { \"types\": \"./widget.d.ts\", \"import\": \"./widget.mjs\" },
        \"./structure\": { \"types\": \"./structure.d.ts\", \"import\": \"./structure.mjs\" },
        \"./resource\": { \"types\": \"./resource.d.ts\", \"import\": \"./resource.mjs\" }
    }
}
")

# 2. Run the build target and place the file safely after compilation
# Define the nested packing directory path
set(NPM_PACK_DIR "${DIST_DIR}/pack")

add_custom_target(
    sss-guis_js_bundle
    ALL
    COMMAND npm run build
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${NPM_PACK_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${DIST_DIR}" "${NPM_PACK_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${NPM_PACK_DIR}/guis.js" "${DIST_DIR}/guis.js"
    COMMAND ${CMAKE_COMMAND} -E remove -f "${NPM_PACK_DIR}/guis.js"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_BINARY_DIR}/package.json.tmp" "${NPM_PACK_DIR}/package.json"
    COMMAND bash -c "PACKED_FILE=$(npm pack \"${NPM_PACK_DIR}\" --loglevel=error | tail -n 1); mv \"$PACKED_FILE\" \"${GENERATED_TARBALL}\""
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GENERATED_INCLUDE_DIRECTORY}/"
    COMMAND bash -c "set -e; set -o pipefail; echo 'unsigned char sss_guis_js[] = {' > ${TEMP_HEADER}; od -v -t x1 -An ${GENERATED_JS} | sed 's/[[:space:]]*\\([0-9a-f]\\{2\\}\\)/0x\\1, /g' >> ${TEMP_HEADER}; sed -i '$ s/, $/};/' ${TEMP_HEADER}; echo -e '\\nunsigned int sss_guis_js_len = sizeof(sss_guis_js);' >> ${TEMP_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TEMP_HEADER}" "${GENERATED_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E remove "${TEMP_HEADER}"
    WORKING_DIRECTORY "${WORKING_DIRECTORY_LOCATION}"
    COMMENT "Generating sss-guis JavaScript bundle"
    VERBATIM
)


set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
    "${GENERATED_JS}" "${GENERATED_DECLARATION}" "${GENERATED_INCLUDE_DIRECTORY}" "${GENERATED_TARBALL}" "${GENERATED_PACKAGE_JSON}"
)
set_property(SOURCE ${GENERATED_HEADER} PROPERTY GENERATED TRUE)
set_property(SOURCE ${GENERATED_DECLARATION} PROPERTY GENERATED TRUE)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE "${GENERATED_INCLUDE_DIRECTORY}")

set(SSS-GUIS-JS-HEADER ${GENERATED_INCLUDE_DIRECTORY} PARENT_SCOPE)
set(SSS-GUIS-NPM-TARBALL ${GENERATED_TARBALL} PARENT_SCOPE)

add_dependencies(${PROJECT_NAME} sss-guis_js_bundle)
