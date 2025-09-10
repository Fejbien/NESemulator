cmake_minimum_required(VERSION 3.10)
project(NESemulator VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(NESemulator main.cpp firstApp.cpp vve_window.cpp)


# Find Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(NESemulator PRIVATE Vulkan::Vulkan)

# Find GLFW3 from Homebrew
find_package(glfw3 REQUIRED)
# Use the modern imported target "glfw::glfw" if available, otherwise fallback to glfw
if(TARGET glfw::glfw)
	target_link_libraries(NESemulator PRIVATE glfw::glfw)
elseif(TARGET glfw)
	target_link_libraries(NESemulator PRIVATE glfw)
else()
	message(FATAL_ERROR "GLFW target not found. Please check your glfw3 installation.")
endif()

# GLM via CPM
include(cmake/CPM.cmake)
# Prevent GLM from adding its test suite (pulls in unsupported headers)
set(GLM_BUILD_TESTS OFF CACHE BOOL "Disable GLM tests" FORCE)
set(GLM_TEST_ENABLE OFF CACHE BOOL "Alias disable tests" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Disable global testing in dependencies" FORCE)
CPMAddPackage("gh:g-truc/glm#1.0.1")
target_link_libraries(NESemulator PRIVATE glm)