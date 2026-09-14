cmake_minimum_required(VERSION 3.10)
project(SeparadorVidro)

# Diz ao CMake onde está a pasta 'build' do OpenCV que acabamos de extrair
set(OpenCV_DIR "C:/opencv/build")

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
add_executable(SeparadorVidro src/main.cpp)
target_link_libraries(SeparadorVidro ${OpenCV_LIBS})