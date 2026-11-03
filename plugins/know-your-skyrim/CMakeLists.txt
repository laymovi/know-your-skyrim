cmake_minimum_required(VERSION 3.21)

project(KnowYourSkyrim VERSION 1.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB sources CONFIGURE_DEPENDS "source/*.cpp" "source/*.h" "source/files/*.cpp" "source/files/*.h" "source/quests/*.cpp" "source/quests/*.h" "source/world/*.cpp" "source/world/*.h")

# The browser draws its models with the code Inventory Reforged draws its panel with, that is the 3D scene of the item
# menus, ground models, faces and bodies for the tile and the inspect view. One source for both DLLs, a fix there
# reaches the tile too. WornView, Copy, Pose and Settings come along with it and stay unused, because the browser shows
# things as they lie and never loads the panel's settings. Stats gives the words of effects. Stacks comes with Profile,
# whose watch the browser never starts, and SafeRead with Stacks. Whom the panel shows is the player here, the browser's
# own Subject.cpp says so
set(panel ../equipment-panel/source)
list(APPEND sources
    ${panel}/Copy.cpp
    ${panel}/Inspect.cpp
    ${panel}/ItemView.cpp
    ${panel}/Pose.cpp
    ${panel}/Profile.cpp
    ${panel}/SafeRead.cpp
    ${panel}/Scene.cpp
    ${panel}/Settings.cpp
    ${panel}/Stacks.cpp
    ${panel}/Stats.cpp
    ${panel}/Translations.cpp
    ${panel}/WornView.cpp
    ${panel}/ZoomKey.cpp
    ../shared/source/Holders.cpp
    ../shared/source/People.cpp
    ../shared/source/Places.cpp)

find_package(CommonLibSSE CONFIG REQUIRED)
find_package(spdlog REQUIRED CONFIG)
find_package(directxtk CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${sources})
target_precompile_headers(${PROJECT_NAME} PRIVATE source/PCH.h)
target_include_directories(${PROJECT_NAME} PRIVATE source source/files source/quests source/world ${panel} ../shared/source)
target_link_libraries(${PROJECT_NAME} PRIVATE CommonLibSSE::CommonLibSSE spdlog::spdlog_header_only)
