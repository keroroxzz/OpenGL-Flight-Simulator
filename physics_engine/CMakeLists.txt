cmake_minimum_required(VERSION 3.0.2)
project(physics_engine)

add_compile_options(-std=c++2a)

set(CMAKE_BINARY_DIR "${CMAKE_SOURCE_DIR}/build")

file(GLOB_RECURSE LIB_SOURCE_FILES ${CMAKE_SOURCE_DIR}/${PROJECT_NAME}/src/*.cpp)
# include_directories("${CMAKE_SOURCE_DIR}/${PROJECT_NAME}")
add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${LIB_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/${PROJECT_NAME}>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/flight_sim/")
# install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
#     RUNTIME DESTINATION bin
#     ARCHIVE DESTINATION lib
#     LIBRARY DESTINATION lib
# )