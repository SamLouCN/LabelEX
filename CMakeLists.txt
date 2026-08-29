# CMakeList.txt: LabelEX 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
project(LabelEX LANGUAGES CXX RC)

# 将源代码添加到此项目的可执行文件。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(UNICODE _UNICODE)

set(SOURCES
	src/main.cpp
	src/PagePicture.cpp
)

set(HEADERS
	src/main.h
	src/resource.h
)

set(RESOURCES
	res/LabelEX.rc
)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/res
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

set_source_files_properties(res/LabelEX.rc PROPERTIES
    VS_RESOURCE_GENERATOR_ADDITIONAL_OPTIONS "/I \"${CMAKE_CURRENT_SOURCE_DIR}/src\"")

target_link_libraries(${PROJECT_NAME}
	gdiplus
	shell32
	comctl32
	shlwapi
	ole32
	strsafe
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>"
)

message(STATUS "LabelEX configured - SDK 10.0.17763")

# TODO: 如有需要，请添加测试并安装目标。
