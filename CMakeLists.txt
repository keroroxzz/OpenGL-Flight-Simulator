cmake_minimum_required(VERSION 3.0.2)
project(flight_sim)

add_compile_options(-std=c++2a)

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")

# OpenGL
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

set(GLFW_INCLUDE_PATH "" CACHE PATH "The directory that contains GL/glfw.h" )
set(GLFW_LIBRARY "" CACHE FILEPATH "libglfw.a or glfw.lib" )

set(GLEW_INCLUDE_PATH "" CACHE PATH "The directory that contains GL/glew.h" )
set(GLEW_LIBRARY "" CACHE FILEPATH "libglew.a or glew.lib" )

set(GLM_INCLUDE_PATH "" CACHE PATH "The directory that contains glm/glm.hpp" )

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.cpp)

include_directories(
	${GLFW_INCLUDE_PATH}
	${GLEW_INCLUDE_PATH}
	${GLM_INCLUDE_PATH}
)

# project headers & interfaces
include_directories(
	"${CMAKE_SOURCE_DIR}/includes"
	"${CMAKE_SOURCE_DIR}"
)

set(LIB_NAME "physics_engine")
add_subdirectory(${LIB_NAME})
link_libraries(${LIB_NAME})
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${GLUT_LIBRARY})
set_target_properties(${PROJECT_NAME} PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/")

