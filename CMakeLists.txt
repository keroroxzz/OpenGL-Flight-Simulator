cmake_minimum_required(VERSION 3.0.2)
project(flight_sim)

add_compile_options(-std=c++11 -g -O0 -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable)

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")

# OpenGL
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(GLEW REQUIRED)

# Add source files
file(GLOB_RECURSE SOURCE_FILES 
	${CMAKE_SOURCE_DIR}/src/*.cpp)
	
# Add header files
file(GLOB_RECURSE HEADER_FILES 
	${CMAKE_SOURCE_DIR}/includes/*.h)

include_directories(
	${GLEW_INCLUDE_DIRS}
)

include_directories(
	"${CMAKE_SOURCE_DIR}/src"
	"${CMAKE_SOURCE_DIR}/includes"
)

# Common source files for both targets
set(COMMON_SOURCES
    src/GPUFluidSolver.cpp
    src/FluidSolver.cpp
    src/Shader.cpp
    src/math3D.cpp
    src/gltools.cpp
    src/Joints.cpp
    src/ObjModel.cpp
    src/PhysicalModel.cpp
)

# Main Simulator
add_executable(${PROJECT_NAME} src/Main.cpp src/Plane.cpp src/FluidSolver.cpp ${COMMON_SOURCES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} GLU pthread)

# Standalone LBM Test
add_executable(lbm_test src/LBMTest.cpp ${COMMON_SOURCES} ${HEADER_FILES})
target_link_libraries(lbm_test ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} GLU pthread)

# Standalone Aircraft Aerodynamics Test
add_executable(aircraft_test src/AircraftTest.cpp ${COMMON_SOURCES} ${HEADER_FILES})
target_link_libraries(aircraft_test ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} GLU pthread)

# Standalone Particle Visual Test
add_executable(particle_test src/ParticleTest.cpp src/Shader.cpp src/math3D.cpp src/gltools.cpp ${HEADER_FILES})
target_link_libraries(particle_test ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} ${GLEW_LIBRARIES} GLU pthread)

set_target_properties(${PROJECT_NAME} PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/")

set_target_properties(lbm_test PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/")

set_target_properties(aircraft_test PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/")

set_target_properties(particle_test PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/")
