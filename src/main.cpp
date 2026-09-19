#include <iostream>
#include <opencv2/opencv.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

using namespace std;
using namespace cv;
using namespace ImGui;

// Inicialização da janela:
GLFWwindow* StartWindow(int width, int height, const char* Esteira) {
    if (!glfwInit()) {
        cerr << "Falha ao inicializar GLFW" << endl;
        return nullptr;
    }

    // Configurações do OpenGL
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(width, height, Esteira, nullptr, nullptr);
    if (!window) {
        cerr << "Falha ao criar a janela GLFW" << endl;
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Habilita V-Sync

    return window;
}

// Inicialização do Contexto do ImGui:
void startImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    CreateContext();
    ImGuiIO& io = GetIO(); (void)io;

    StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// Renderização da interface do ImGui:
void desenharInterface() {
    Begin("Interface de Controle da Esteira");
    Text("Bem-vindo ao sistema de controle da esteira!");
    Text("Aqui você pode monitorar e controlar a esteira.");
    Separator();
    End();
}

// Limpeza e encerramento do ImGui:
void EndOperation(GLFWwindow* window) {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

int main() {

    // Inicializa a janela
    GLFWwindow* window = StartWindow(800, 600, "Teste");
    if (!window) return -1;

    startImGui(window);

    // Cor de fundo da janela
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        // Processa eventos do GLFW
        glfwPollEvents();

        // Inicia um novo frame do ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        NewFrame();

        // Desenha a interface do ImGui
        desenharInterface();

        // Renderiza o ImGui
        Render();

        // Aplica cor e lipa o buffer
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        //Desenha os dados do ImGui usando o backend do OpenGL3
        ImGui_ImplOpenGL3_RenderDrawData(GetDrawData());

        // Troca os buffers da janela
        glfwSwapBuffers(window);
    }

    EndOperation(window);
 // Teste inicial de contorno

    //Etapa 1: Lê as imagens
    Mat branco = imread("C:/Users/phlea/Downloads/IFSC/PIE/PI-Esteira_Separadora/imagens/branca.jpg", IMREAD_COLOR);
    Mat manchas = imread("C:/Users/phlea/Downloads/IFSC/PIE/PI-Esteira_Separadora/imagens/manchas.jpg", IMREAD_COLOR);

    if (branco.empty()) {
        cout << "ERRO: Imagem nao encontrada no caminho especificado!" << endl;
        return -1;
    }

    if (manchas.empty()) {
        cout << "ERRO: Imagem 2 nao encontrada no caminho especificado!" << endl;
        return -1;
    }

    //Etapa 2: Converte as imagens para escala de cinza
    Mat brancoGray, manchasGray;
    cvtColor(branco, brancoGray, COLOR_BGR2GRAY);
    cvtColor(manchas, manchasGray, COLOR_BGR2GRAY);

    //Etapa 3: Gera uma imagem com a diferença absoluta das
    Mat diferenca;
    absdiff(brancoGray, manchasGray, diferenca);

    //Etapa 4: Aplica um limiar para destacar as diferenças
         // Tudo que tiver uma diferença de pixel maior que 30 vira branco (255)
    Mat mascara;
    threshold(diferenca, mascara, 30, 255, THRESH_BINARY);

    // Etapa 5: Encontrar os contornos na máscara
    vector<vector<Point>> contornos;
        // RETR_EXTERNAL: pega apenas os contornos externos (ignora furos dentro do objeto)
    findContours(mascara, contornos, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    // Etapa 6: Cria a imagem com os contornos
    Mat resultado = manchas.clone(); // Cria uma cópia da imagem com os objetos
        // -1 significa desenhar TODOS os contornos. Scalar(0, 0, 255) é a cor vermelha em BGR
    drawContours(resultado, contornos, -1, Scalar(0, 0, 255), 2);

    //Etapa final: Mostra todas as imagens e informa quantos objetos foram detectados

    cout << "Objetos detectados: " << contornos.size() << endl;

    namedWindow("padrao", WINDOW_AUTOSIZE);
    imshow("padrao", branco);

    namedWindow("manchas", WINDOW_AUTOSIZE);
    imshow("manchas", manchas);

    namedWindow("diferenca", WINDOW_AUTOSIZE);
    imshow("diferenca", diferenca);

    namedWindow("mascara", WINDOW_AUTOSIZE);
    imshow("mascara", mascara);
    
    namedWindow("contornos", WINDOW_AUTOSIZE);
    imshow("contornos", resultado);

    waitKey(0); // Espera por uma tecla ser pressionada
    destroyAllWindows(); // Fecha todas as janelas abertas

    // Fim teste

    return 0;
}