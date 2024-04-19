cmake_minimum_required(VERSION 3.0.2)
project(render_engine)

# ======== sub-projects ========
# add your suv-projects here
# =============================
# add_subdirectory(sub_project_name)
# link_libraries(${SHARED_LIBS})
# =============================

include_directories(
	"${CMAKE_SOURCE_DIR}"
)

link_libraries(${SHARED_LIBS})

# add source files
file(GLOB_RECURSE LIB_SOURCE_FILES src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${LIB_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/${PROJECT_NAME}>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(${PROJECT_NAME} PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${OUTPUT_FOLDER}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${OUTPUT_FOLDER}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${OUTPUT_FOLDER}/")

set(SHARED_LIBS ${SHARED_LIBS} ${PROJECT_NAME} PARENT_SCOPE)