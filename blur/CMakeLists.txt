find_package(OpenCL REQUIRED)
include_directories(${OpenCL_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/libraries)
include_directories(${CMAKE_SOURCE_DIR}/..)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/resources/source.bmp DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/)

add_executable("parallel-example-001-blur" impl.cpp main.cpp)
target_link_libraries("parallel-example-001-blur" ${OpenCL_LIBRARY})

