cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_FLAGS "-std=c++17")
project(client)
set(EXECUTABLE_OUTPUT_PATH ../)
#头文件目录
include_directories(../include)
#源文件目录
aux_source_directory(. CUR_DIR_SRC)
#设置环境变量
SET(CLIENT ${CUR_DIR_SRC})
#生成可执行文件
add_executable(${PROJECT_NAME} ${CLIENT})
#链接库文件
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} crypto)
target_link_libraries(${PROJECT_NAME} redis++)
target_link_libraries(${PROJECT_NAME} hiredis)
target_link_libraries(${PROJECT_NAME} readline)
