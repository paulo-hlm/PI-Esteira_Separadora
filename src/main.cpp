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

    // Abre a interface em tela cheia
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

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

    // Centraliza o título da janela
    GetStyle().WindowTitleAlign = ImVec2(0.5f, 0.5f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

// Função para centralizar o texto
void TextoCentralizado(const char* texto) {
    // Calcula o tamanho da janela e o tamanho do texto
    float larguraJanela = ImGui::GetWindowWidth();
    float larguraTexto = ImGui::CalcTextSize(texto).x;

    // Define a nova posição X do cursor
    ImGui::SetCursorPosX((larguraJanela - larguraTexto) * 0.5f);
    
    // Desenha o texto
    ImGui::Text("%s", texto);
}

// Função para centralizar o texto colorido
void TextoColoridoCentralizado(ImVec4 cor, const char* texto) {
    float larguraJanela = ImGui::GetWindowWidth();
    float larguraTexto = ImGui::CalcTextSize(texto).x;

    ImGui::SetCursorPosX((larguraJanela - larguraTexto) * 0.5f);
    ImGui::TextColored(cor, "%s", texto);
}

// Renderização da interface do ImGui:
void desenharInterface(GLuint texBranco, GLuint texManchas, GLuint texResultado, int numContornos, bool& estadoEsteira, double tempo, int& deteccao, int& total_V, int& total_O) {

    // Autores
    SetNextWindowPos(ImVec2(1180, 150), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 200), ImGuiCond_Once);
    Begin("Autores:", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    End();
    

    // Tempo
    int horas = (int)tempo / 3600;
    int minutos = ((int)tempo % 3600) / 60;
    int segundos = (int)tempo % 60;
    SetNextWindowPos(ImVec2(1180, 10), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 50), ImGuiCond_Once);
    Begin("Tempo de operação", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    // Monta o texto do cronômetro antes de enviar para centralizar
    char bufferCronometro[64];
    snprintf(bufferCronometro, sizeof(bufferCronometro), "%02d:%02d:%02d", horas, minutos, segundos);
    TextoColoridoCentralizado(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), bufferCronometro);
    End();

    // Imagens
    SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(750, 470), ImGuiCond_Once);
    Begin("Visualização das imagens", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    Text("          Referência:                         Comparação:                           Resultado:");
    if (texBranco) Image((void*)(intptr_t)texBranco, ImVec2(236, 419));
    SameLine(); 
    if (texManchas) Image((void*)(intptr_t)texManchas, ImVec2(236, 419));
    SameLine(); 
    if (texResultado) Image((void*)(intptr_t)texResultado, ImVec2(236, 419));
    End();
   
    // Botão de controle da esteira
    ImGuiWindowFlags flagsBotao = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    SetNextWindowPos(ImVec2(1180, 60), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 80), ImGuiCond_Once);
    Begin("Interface de Controle da Esteira", NULL, flagsBotao);
    PushStyleVar(ImGuiStyleVar_FrameRounding, 25.0f); 
    if (estadoEsteira == true){
        PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); // Vermelho
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (Button("STOP", ImVec2(130, 50))) {
            estadoEsteira = false;
        }
        PopStyleColor(2);
    } else {
        PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.8f, 0.1f, 1.0f)); // Verde
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
        if (Button("START", ImVec2(130, 50))) {
            estadoEsteira = true;
        }
        PopStyleColor(2);
    }
    PopStyleVar(1);
    End();

    //Status da esteira
    SetNextWindowPos(ImVec2(770, 10), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Once);
    Begin("Status", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    Text("Estado da esteira: %s", estadoEsteira ? "Ligada" : "Desligada");
    Separator();

    if (estadoEsteira == false) {
        Text("Inicie a esteira para iniciar a operação");
    } else {
        if (deteccao == 0) {
        Text("Aguardando Imagem");
        } else if (deteccao == 1) {
            Text("Objeto detectado:");
            SameLine();
            TextColored(ImVec4(0.1f, 0.8f, 0.1f, 1.0f), "VIDRO");
            total_V++
        } else if (deteccao == 2){
            Text("Objeto detectado:");
            SameLine();
            TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "Descarte");
            total_O++
        }
    }
    Separator();

    PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (Button("teste", ImVec2(130, 50))) {
        if (deteccao != 2) {
            deteccao++;
        } else {
            deteccao = 0;
        }
        }
        PopStyleColor(2);

    End();
    
    // Contagem de objetos
    SetNextWindowPos(ImVec2(770, 170), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(400, 50), ImGuiCond_Once);
    Begin("Contagem de objetos:", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    Text("Número de objetos detectados totais: X");
    Text("Número de objetos detectados em frame: %d", numContornos);
    Separator();
    Text("Número de objetos transparentes detectados: X");
    Text("Número de objetos opacos detectados: X");
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

// Converte uma imagem de OpenCV (Mat) em uma textura OpenGL (GLuint)
GLuint CVtoGL(const Mat& mat) {
    if (mat.empty()) return 0;

    Mat imagemConvertida;
    // Converção do padrão do OpenCV (RGB ou Grayscale) para RGBA.
    if (mat.channels() == 3) {
        cvtColor(mat, imagemConvertida, COLOR_BGR2RGBA);
    } else if (mat.channels() == 1) {
        cvtColor(mat, imagemConvertida, COLOR_GRAY2RGBA);
    } else {
        imagemConvertida = mat.clone();
    }

    // Gera o ID da textura no OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configura os filtros de redimensionamento (suavização)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Evita problemas de alinhamento de memória entre OpenCV e OpenGL
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Faz o upload dos pixels da memória RAM (OpenCV) para a VRAM (Placa de Vídeo/OpenGL)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imagemConvertida.cols, imagemConvertida.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagemConvertida.ptr());

    return textureID;
}

int main() {

    // Variáveis
    bool estadoEsteira = false;
    double tempoOperacao = 0.0;
    double tempoJanela = glfwGetTime();
    int deteccao = 0;
    int total_V = 0;
    int total_O = 0;
    
    //Leitura as imagens
    Mat branco = imread("C:/Users/phlea/Downloads/IFSC/PIE/PI-Esteira_Separadora/imagens/branca.jpg", IMREAD_COLOR);
    Mat manchas = imread("C:/Users/phlea/Downloads/IFSC/PIE/PI-Esteira_Separadora/imagens/manchas.jpg", IMREAD_COLOR);
    if (branco.empty() || manchas.empty()) {
        cerr << "Erro ao carregar as imagens!" << endl;
        return -1;
    }

    Mat brancoGray, manchasGray, diferenca, mascara;
    cvtColor(branco, brancoGray, COLOR_BGR2GRAY);
    cvtColor(manchas, manchasGray, COLOR_BGR2GRAY);
    absdiff(brancoGray, manchasGray, diferenca);
    threshold(diferenca, mascara, 30, 255, THRESH_BINARY);

    vector<vector<Point>> contornos;
    findContours(mascara, contornos, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

    Mat resultado = manchas.clone();
    drawContours(resultado, contornos, -1, Scalar(0, 0, 255), 2);

    // Inicializa a janela
    GLFWwindow* window = StartWindow(800, 600, "Sistema de filtragem de vidro para reciclagem");
    if (!window) return -1;

    startImGui(window);

    GLuint textureBranco = CVtoGL(branco);
    GLuint textureManchas = CVtoGL(manchas);
    GLuint textureResultado = CVtoGL(resultado);

    // Cor de fundo da janela
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

    // Loop principal de exibição
    while (!glfwWindowShouldClose(window)) {

        // Lógica botão
        double tempoAtual = glfwGetTime();
        if (estadoEsteira == true){ 
            tempoOperacao += (tempoAtual - tempoJanela);
        }
        tempoJanela = tempoAtual;
        
        // Processa eventos do GLFW
        glfwPollEvents();

        // Inicia um novo frame do ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        NewFrame();

        // Desenha a interface do ImGui
        desenharInterface(textureBranco, textureManchas, textureResultado, (int)contornos.size(), estadoEsteira, tempoOperacao, deteccao, total_V, total_O);

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

    return 0;
}
