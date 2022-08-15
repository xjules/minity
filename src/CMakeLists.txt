file(GLOB_RECURSE minity_sources *.cpp *.h)
add_executable(minity ${minity_sources})
set_target_properties(minity PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(glm CONFIG REQUIRED)
target_link_libraries(minity PRIVATE glm::glm)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(minity PRIVATE glfw)

find_package(glbinding CONFIG REQUIRED)
target_link_libraries(minity PRIVATE glbinding::glbinding glbinding::glbinding-aux)

find_package(globjects CONFIG REQUIRED)
target_link_libraries(minity PRIVATE globjects::globjects)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(minity PRIVATE imgui::imgui)

find_package(tinyfiledialogs CONFIG REQUIRED)
target_link_libraries(minity PRIVATE tinyfiledialogs::tinyfiledialogs)

find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(minity PRIVATE ${STB_INCLUDE_DIRS})
