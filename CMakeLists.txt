cmake_minimum_required(VERSION 3.10)

project(ttne-display)

include(cmake/GitVersion.cmake)

option(SIMULATOR_ENABLED "Simulator enabled" OFF)
set(LV_CONF_BUILD_DISABLE_EXAMPLES 1)
set(LV_CONF_BUILD_DISABLE_DEMOS 1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_BUILD_TYPE Debug)

add_compile_options(-Wall)
add_compile_options(-Wextra)
add_compile_options(-Wpedantic)

if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json" )
  EXECUTE_PROCESS( COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json
  )
ENDIF()

if(ASSET_PATH)
add_compile_definitions(ASSET_PATH=${ASSET_PATH})
endif(ASSET_PATH)

get_git_version(GIT_VERSION)
add_definitions("-DGIT_VERSION=\"${GIT_VERSION}\"")

if(SIMULATOR_ENABLED)
add_compile_definitions(SIMULATOR_ENABLED)
find_package(SDL2 REQUIRED SDL2)
find_package(Freetype REQUIRED)
include_directories(
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}/../
        ${PROJECT_SOURCE_DIR}
    )
else()
    include_directories(
        ${PROJECT_SOURCE_DIR}
    )
endif(SIMULATOR_ENABLED)

find_package(CURL REQUIRED)
find_package(cJSON REQUIRED)

add_subdirectory(lvgl)
add_subdirectory(lv_drivers)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(${CMAKE_SOURCE_DIR}/include)
file(GLOB PROJECT_SRC "src/*.c")
add_executable(cmdisplay ${PROJECT_SRC} ${SOURCES} ${INCLUDES})
target_include_directories(lvgl PRIVATE ${FREETYPE_INCLUDE_DIRS})
include_directories(include)
include_directories(${CURL_INCLUDE_DIR})
include_directories(${CJSON_INCLUDE_DIR})

target_include_directories(lvgl SYSTEM PUBLIC include)

target_link_libraries(cmdisplay PRIVATE lvgl lv_drivers ${CURL_LIBRARIES} ${CJSON_LIBRARIES})
if(SIMULATOR_ENABLED)
    target_link_libraries(cmdisplay PRIVATE ${SDL2_LIBRARIES} ${FREETYPE_LIBRARIES})
endif(SIMULATOR_ENABLED)

add_custom_target(run COMMAND ${EXECUTABLE_OUTPUT_PATH}/cmdisplay)

install(TARGETS cmdisplay DESTINATION bin)
