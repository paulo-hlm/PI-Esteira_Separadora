cmake_minimum_required(VERSION 3.10)
project(SeparadorVidro)

# 1. Habilita o módulo de download do CMake
include(FetchContent)

# 2. Desativa a compilação de testes e exemplos do GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# 3. Baixa o GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
FetchContent_MakeAvailable(glfw)

# 4. Configuração do OpenCV
set(OpenCV_DIR "C:/opencv/build")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# 5. Cria o executável APENAS com o seu código fonte principal por enquanto
add_executable(SeparadorVidro src/main.cpp)

# 6. Configuração do ImGui
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)

# Lista de forma explícita ao invés de usar file(GLOB)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# 7. Adiciona os arquivos do ImGui ao executável que já foi criado
target_sources(SeparadorVidro PRIVATE ${IMGUI_SOURCES})
target_include_directories(SeparadorVidro PRIVATE 
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
)

# 8. Linka TUDO de uma vez só no final (OpenCV, GLFW e OpenGL)
target_link_libraries(SeparadorVidro PRIVATE ${OpenCV_LIBS} glfw opengl32)