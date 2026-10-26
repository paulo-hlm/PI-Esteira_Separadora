#define NOMINMAX
#include <windows.h>

#include <iostream>
#include <vector>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
 
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp> 

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>

#define BAUDRATE 9600
#define contraste 20
#define tamMinimo 500
#define ruido 10
#define Limiar 70
#define tempoTela 1000

HANDLE hSerial;

using namespace std;
using namespace cv;
using namespace ImGui;

// Inicia a comunicação serial com a porta especificada e configura os parâmetros de comunicação
bool InicializarSerial(const char* portaCOM, int baudRate) {
    // Abre a conexão com a porta especificada
    hSerial = CreateFileA(portaCOM, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Configura os parâmetros de comunicação (Baud Rate, 8 bits de dados, 1 stop bit, sem paridade)
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return false;
    }

    dcbSerialParams.BaudRate = baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return false;
    }

    // Define os timeouts para garantir que a thread principal não trave
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        CloseHandle(hSerial);
        return false;
    }

    return true;
}

// Comando Serial
void EnviarComandoSerial(char comando) {
    if (hSerial != INVALID_HANDLE_VALUE) {
        DWORD bytesEscritos;
        
        // Envia o caractere (1 byte) e registra o total de bytes efetivamente transmitidos
        if (!WriteFile(hSerial, &comando, 1, &bytesEscritos, NULL)) {
            cerr << "Erro ao enviar comando serial: " << comando << endl;
        }
    }
}

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

// Estados
enum Estados {
    PREPARACAO,
    CAMERAON,
    LUZON,
    ESTEIRAOFF,
    SEMPECA,
    PECAIDENTIFICADA,
    VIDRO,
    DESCARTE,
    ERRO
};

// Auxiliar para ter o texto do estado
const char* status(Estados estado) {
    switch (estado) {
        case PREPARACAO:       return "PREPARACAO";
        case CAMERAON:         return "CAMERAON";
        case LUZON:            return "LUZON";
        case ESTEIRAOFF:       return "ESTEIRAOFF";
        case SEMPECA:          return "SEMPECA";
        case PECAIDENTIFICADA: return "PECAIDENTIFICADA";
        case VIDRO:            return "VIDRO";
        case DESCARTE:         return "DESCARTE";
        case ERRO:             return "ERRO";
        default:               return "DESCONHECIDO";
    }
}

void Visao(Estados& _estado, Estados& _estadoAnterior, bool& estadoCamera, bool& estadoLuz, bool& estadoEsteira, Mat& frame, Mat& frameReferencia, Mat& frameDiff, Mat& frameThresh, Mat& frameMorph, int& numContornos, int& total_V, int& total_O){

        bool pecadetectada = false;
        int tipo = 0; // 0 - nada; 1 - vidro; 2 - descarte;
        
        if (estadoEsteira && !frame.empty() && !frameReferencia.empty()) {
            cv::Mat grayAtual, grayRef;
            
            // 1. Converte ambos para escala de cinza
            cv::cvtColor(frame, grayAtual, cv::COLOR_BGR2GRAY);
            cv::cvtColor(frameReferencia, grayRef, cv::COLOR_BGR2GRAY);
        
            // 2. Aplica desfoque para suavizar ruídos da esteira
            cv::GaussianBlur(grayAtual, grayAtual, cv::Size(5, 5), 0);
            cv::GaussianBlur(grayRef, grayRef, cv::Size(5, 5), 0);

            // 3. Diferença Absoluta
            cv::absdiff(grayRef, grayAtual, frameDiff);
            cv::addWeighted(frameDiff, 3.0, frameDiff, 0, 0, frameDiff);

            // 4. Limiarização
            cv::threshold(frameDiff, frameThresh, 90, 255, cv::THRESH_BINARY);

            // 5. Fechamento Morfológico 
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(15, 15));
            cv::morphologyEx(frameThresh, frameMorph, cv::MORPH_CLOSE, kernel);

           // 6. Encontra os contornos dos fragmentos
            std::vector<std::vector<cv::Point>> contornos;
            cv::findContours(frameMorph, contornos, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            // Agrupar todos os pontos válidos num único vector
            std::vector<cv::Point> todosOsPontos;
            
            for (size_t i = 0; i < contornos.size(); i++) {
                if (contornos[i].size() > 5) {
                    todosOsPontos.insert(todosOsPontos.end(), contornos[i].begin(), contornos[i].end());
                }
            }

            numContornos = 0;

            if (todosOsPontos.size() > 50) {
                
                // ABORDAGEM MANUAL: Encontra os limites mínimo e máximo (substitui o boundingBox e minAreaRect)
                int minX = 99999, maxX = -1;
                int minY = 99999, maxY = -1;
                
                for (const auto& p : todosOsPontos) {
                    if (p.x < minX) minX = p.x;
                    if (p.x > maxX) maxX = p.x;
                    if (p.y < minY) minY = p.y;
                    if (p.y > maxY) maxY = p.y;
                }

                int largura = maxX - minX;
                int altura = maxY - minY;
                int areaAprox = largura * altura; // Aproximação retangular da área
                    
                // Filtra ruídos usando a área aproximada
                if (areaAprox > tamMinimo) { 
                    
                    // Cálculo manual da proporção
                    float proporcao = 0.0f;
                    if (altura > 0 && largura > 0) {
                        proporcao = (largura < altura) ? ((float)largura / altura) : ((float)altura / largura);
                    }

                    // Verificação de borda com base nos limites encontrados
                    bool tocaBorda = (minX <= 5 || minY <= 5 || 
                                     maxX >= (frame.cols - 5) || 
                                     maxY >= (frame.rows - 5));

                    if (proporcao > 0.3f && proporcao < 3.0f) {
                        numContornos = 1; 
                        pecadetectada = true;

                        // Cria a caixa delimitadora com base nos pontos extremos
                        cv::Rect boundingBox(minX, minY, largura, altura);

                        if (tocaBorda) {
                            // Objeto a entrar/sair: retângulo amarelo
                            cv::rectangle(frame, boundingBox, cv::Scalar(0, 255, 255), 2);
                        } else {
                            // Cria a máscara sólida preenchendo o retângulo para extrair o brilho
                            cv::Mat mascara = cv::Mat::zeros(frameMorph.size(), CV_8UC1);
                            cv::rectangle(mascara, boundingBox, cv::Scalar(255), cv::FILLED);

                            cv::Scalar mediaBrilho = cv::mean(grayAtual, mascara);
                            
                            if (mediaBrilho[0] > contraste) { 
                                tipo = 1; // Vidro
                                cv::rectangle(frame, boundingBox, cv::Scalar(0, 255, 0), 2); // Verde
                            } else {
                                tipo = 2; // Descarte
                                cv::rectangle(frame, boundingBox, cv::Scalar(0, 0, 255), 2); // Vermelho
                            }
                        }
                    }
                }
            } else {
                tipo = 0;
            }
        }
            
    switch (_estado){
        
        // Estado inicial, a interface foi gerada mas nenhuma ação foi tomada
        case PREPARACAO:
            estadoCamera = false;
            estadoLuz = false;
            estadoEsteira = false;
            break;

        // A câmera foi iniciada e a imagem está sendo mostrada na tela
        case CAMERAON:
            estadoCamera = true;
            estadoLuz = false;
            estadoEsteira = false;
        break;

        case LUZON:
            estadoCamera = false;
            estadoLuz = true;
            estadoEsteira = false;
        break;
        
        // A câmera e a iluminação estão ligadas, aguarda apenas o comando da esteira
        case ESTEIRAOFF:
            estadoCamera = true;
            estadoLuz = true;
            estadoEsteira = false;
        break;

        // A esteira foi iniciada e o sistema de visão entrou em operação
        // envia sinal para a catraca até que identifique um contorno fechado
        case SEMPECA:
            
            estadoCamera = true;
            estadoLuz = true;
            estadoEsteira = true;
            
            if (tipo == 1){
                EnviarComandoSerial('T'); // Objeto fechado na tela: trava a catraca
                EnviarComandoSerial('V'); // Processamento concluído: separa vidro
                _estado = VIDRO;
                _estadoAnterior = SEMPECA;
                total_V++;
            }
            else if (tipo == 2){
                EnviarComandoSerial('T'); // Objeto fechado na tela: trava a catraca
                EnviarComandoSerial('D'); // Processamento concluído: separa descarte
                total_O++;
                _estado = DESCARTE;
                _estadoAnterior = SEMPECA;
            }
        break;

        // Classifica o contorno baseado em contraste e adiciona ao contador
        // Envia sinal para fechar a catraca e orientar o separador
        case VIDRO:
            if (tipo == 0) {
                EnviarComandoSerial('L'); // Objeto saiu da visão: libera próxima peça
                _estado = SEMPECA;
                _estadoAnterior = VIDRO;
            }
        break;

        // Classifica o contorno baseado em contraste e adiciona ao contador
        // Envia sinal para fechar a catraca e orientar o separador
        case DESCARTE:
            if (tipo == 0) {
                EnviarComandoSerial('L'); // Objeto saiu da visão: libera próxima peça
                _estado = SEMPECA;
                _estadoAnterior = DESCARTE;
            }
        break;

        // Gera mensagens de erro e orientações
        // Trava a esteira
        case ERRO:
            EnviarComandoSerial('P'); // Garante que a esteira pare
            EnviarComandoSerial('T'); // Garante que a catraca não libere peças
        break;
    }    
}

// Renderização da interface do ImGui:
void desenharInterface(Estados& _estado, Estados& _estadoAnterior, GLuint textCamera, GLuint textReferencia, GLuint textDiff, GLuint textThresh, GLuint textMorph, int numContornos, bool& estadoEsteira, bool& estadoCamera, bool&estadoCameraAnterior, bool& estadoLuz, double tempo, int& total_V, int& total_O) {

    // Estados
    SetNextWindowPos(ImVec2(1180, 450), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 80), ImGuiCond_Once);
    Begin("Estados:", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "%s", status(_estado));
    Separator();
    TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", status(_estadoAnterior));
    End();

    // Autores
    SetNextWindowPos(ImVec2(1180, 150), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 120), ImGuiCond_Once);
    Begin("Autores:", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    Separator();
    Text("  João Victor");
    Text("  Paulo Henrique");
    Text("  Theo de Andrade");
    Text("  Vitor Pazetto");
    Separator();
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

    // Câmera ao vivo
    SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(750, 530), ImGuiCond_Once);
    Begin("Live camera", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    
    if (estadoCamera){
        if (textCamera){
            SetCursorPosX((750 - 640) * 0.5f); // Centraliza a imagem da câmera
            Image((void*)(intptr_t)textCamera, ImVec2(640, 480));
        } else {
            SetCursorPosY((530 - 20) * 0.5f); // Centraliza o texto
            TextoCentralizado("Sem sinal de imagem.");
        }
    } else {
        SetCursorPosY((530 - 20) * 0.5f); // Centraliza o texto
        TextoCentralizado("Câmera desligada, aguardando comando.");
    }

    End();

    // Debug do processamento de visão
    SetNextWindowPos(ImVec2(10, 550), ImGuiCond_Once); 
    SetNextWindowSize(ImVec2(780, 240), ImGuiCond_Once); // Aumentamos a largura para acomodar 3 imagens
    Begin("Debug do Processamento de Visao");
    
    if (estadoEsteira && textDiff && textThresh && textMorph) {
        Text("Subtracao (AbsDiff)");
        SameLine(260);
        Text("Limiar (Com Ruido)");
        SameLine(510);
        Text("Morfologia (Limpo)");
        
        // Exibe as três imagens lado a lado (Tamanho ajustado para caberem)
        Image((void*)(intptr_t)textDiff, ImVec2(240, 180));
        SameLine(260);
        Image((void*)(intptr_t)textThresh, ImVec2(240, 180));
        SameLine(510);
        Image((void*)(intptr_t)textMorph, ImVec2(240, 180));
    } else {
        SetCursorPosY(100);
        TextoCentralizado("Inicie a esteira para visualizar o processamento.");
    }
    End();
   
    // Botão de controle da esteira
    ImGuiWindowFlags flagsBotaoE = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    SetNextWindowPos(ImVec2(1180, 70), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 80), ImGuiCond_Once);
    Begin("Interface de Controle da Esteira", NULL, flagsBotaoE);
    PushStyleVar(ImGuiStyleVar_FrameRounding, 25.0f); 
    if (estadoEsteira == true){
        PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); // Vermelho
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (Button("STOP", ImVec2(130, 50))) {
            EnviarComandoSerial('P'); // Para o motor da esteira
            EnviarComandoSerial('T'); // Trava a catraca para reter a fila
            _estadoAnterior = _estado;
            _estado = ESTEIRAOFF;
            estadoCameraAnterior = false;
        }
        PopStyleColor(2);
    } else {
        PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.8f, 0.1f, 1.0f)); // Verde
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
        if (Button("START", ImVec2(130, 50))) {
            if (_estado == ESTEIRAOFF){
                EnviarComandoSerial('E'); // Liga o motor da esteira
                EnviarComandoSerial('L'); // Libera a entrada da primeira peça
                _estado = SEMPECA;
                _estadoAnterior = ESTEIRAOFF;
            } else {
                _estadoAnterior = _estado;
                _estado = ERRO;
            }
        }
        PopStyleColor(2);
    }
    PopStyleVar(1);
    End();

    // Botão de controle da câmera
    ImGuiWindowFlags flagsBotaoC = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    SetNextWindowPos(ImVec2(1180, 280), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 80), ImGuiCond_Once);
    Begin("Interface de Controle da Câmera", NULL, flagsBotaoC);
    PushStyleVar(ImGuiStyleVar_FrameRounding, 25.0f); 
    if (estadoCamera == true){
        PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.1f, 0.1f, 1.0f)); // Vermelho
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
        if (Button("Câmera", ImVec2(130, 50))) {
            if (_estado == ESTEIRAOFF){
                _estadoAnterior = ESTEIRAOFF;
                _estado = LUZON;
            } else if (_estado == CAMERAON){
                _estadoAnterior = CAMERAON;
                _estado = PREPARACAO;
            } else {
                _estadoAnterior = _estado;
                _estado = ERRO;
            }
        }
        PopStyleColor(2);
    } else {
        PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.8f, 0.1f, 1.0f)); // Verde
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 1.0f, 0.2f, 1.0f));
        if (Button("Câmera", ImVec2(130, 50))) {
            if (_estado == PREPARACAO){
                _estadoAnterior = PREPARACAO;
                _estado = CAMERAON;
            } else if (_estado == LUZON){
                _estadoAnterior = LUZON;
                _estado = ESTEIRAOFF;
            } 
        }
        PopStyleColor(2);
    }
    PopStyleVar(1);
    End();

    // Botão de controle da luz
    ImGuiWindowFlags flagsBotaoL = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    SetNextWindowPos(ImVec2(1180, 360), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(150, 80), ImGuiCond_Once);
    Begin("Interface de Controle da Luz", NULL, flagsBotaoL);
    PushStyleVar(ImGuiStyleVar_FrameRounding, 25.0f); 
    
    if (estadoLuz == true){
        PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // Texto Preto
        PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Amarelo
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
        
        if (Button("Iluminação", ImVec2(130, 50))) {
            EnviarComandoSerial('O'); // Acende a iluminação
            if (_estado == ESTEIRAOFF){
                _estadoAnterior = ESTEIRAOFF;
                _estado = CAMERAON;
            } else if (_estado == LUZON){
                _estadoAnterior = LUZON;
                _estado = PREPARACAO;
            } else {
                _estadoAnterior = _estado;
                _estado = ERRO;
            }
        }
        PopStyleColor(3); 
    } else {
        PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 1.0f)); // Preto
        PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        
        if (Button("Iluminação", ImVec2(130, 50))) { 
            EnviarComandoSerial('A'); // Acende a iluminação
            if (_estado == PREPARACAO){
                _estadoAnterior = PREPARACAO;
                _estado = LUZON;
            } else if (_estado == CAMERAON){
                _estadoAnterior = CAMERAON;
                _estado = ESTEIRAOFF;
            } 
        }
        PopStyleColor(2); 
    }
    
    PopStyleVar(1);
    End();

    //Referência
    SetNextWindowPos(ImVec2(770, 380), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(400, 160), ImGuiCond_Once);
    Begin("Imagem de Referência", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    if (estadoEsteira && textReferencia){
        SetCursorPosX((400 - 320) * 0.5f); // Centraliza a imagem da referência
        Image((void*)(intptr_t)textReferencia, ImVec2(320, 120));
    } else {
        SetCursorPosY((160 - 20) * 0.5f); // Centraliza o texto
        TextoCentralizado("Inicie a esteira para capturar a referência.");
    }
    End();

    //Status da esteira
    SetNextWindowPos(ImVec2(770, 10), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Once);
    Begin("Status", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    Text("Estado da esteira: %s", estadoEsteira ? "Ligada" : "Desligada");
    Separator();
    Text("Estado da câmera: %s", estadoCamera ? "Ligada" : "Desligada");
    Separator();
    Text("Estado da iluminação: %s", estadoLuz ? "Ligada" : "Desligada");
    Separator();

    if (estadoEsteira == false) {
        Text("Inicie a esteira para iniciar a operação");
    } else {
        if (_estado == SEMPECA) {
        Text("Aguardando Imagem");
        } else if (_estado == VIDRO) {
            Text("Objeto detectado:");
            SameLine();
            TextColored(ImVec4(0.1f, 0.8f, 0.1f, 1.0f), "VIDRO");
        } else if (_estado == DESCARTE){
            Text("Objeto detectado:");
            SameLine();
            TextColored(ImVec4(0.8f, 0.1f, 0.1f, 1.0f), "DESCARTE");
        }
    }

    End();
    
    // Contagem de objetos
    SetNextWindowPos(ImVec2(770, 170), ImGuiCond_Once);
    SetNextWindowSize(ImVec2(400, 200), ImGuiCond_Once);
    Begin("Contagem de objetos:", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    Text("Número de objetos detectados totais: %d", total_V+total_O);
    Text("Número de objetos detectados em frame: %d", numContornos);
    Separator();
    Text("Número de objetos transparentes detectados: %d", total_V);
    Text("Número de objetos opacos detectados: %d", total_O);
    Separator();
    End();

    // Mensagem de erro
    if (_estado == ERRO) {
        OpenPopup("Erro");
    }    

    if(BeginPopupModal("Erro", NULL, ImGuiWindowFlags_AlwaysAutoResize)){
        if (_estadoAnterior == PREPARACAO) {
            TextoColoridoCentralizado(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Para iniciar a esteira, a câmera e a iluminação devem estar ligadas.");
            Spacing();
            SetCursorPosX((GetWindowWidth() - 120) * 0.5f);
            if (Button("OK", ImVec2(120, 0))) {
                _estado = PREPARACAO;
                _estadoAnterior = ERRO;
                CloseCurrentPopup();
            }
        } else if (_estadoAnterior == CAMERAON) {
            TextoColoridoCentralizado(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Para iniciar a esteira a iluminação deve estar ligada.");
            Spacing();
            SetCursorPosX((GetWindowWidth() - 120) * 0.5f);
            if (Button("OK", ImVec2(120, 0))) {
                _estado = CAMERAON;
                _estadoAnterior = ERRO;
                CloseCurrentPopup();
            }
        } else if (_estadoAnterior == LUZON) {
            TextoColoridoCentralizado(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Para iniciar a esteira a câmera deve estar ligada.");
            Spacing();
            SetCursorPosX((GetWindowWidth() - 120) * 0.5f);
            if (Button("OK", ImVec2(120, 0))) {
                _estado = LUZON;
                _estadoAnterior = ERRO;
                CloseCurrentPopup();
            }
        }else if (estadoEsteira == true) {
            TextoColoridoCentralizado(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Pare a esteira antes de interromper a câmera ou a iluminação.");
            Spacing();
            SetCursorPosX((GetWindowWidth() - 120) * 0.5f);
            if (Button("OK", ImVec2(120, 0))) {
                _estado = _estadoAnterior;
                _estadoAnterior = ERRO;
                CloseCurrentPopup();
            }
        }
        EndPopup();
    }

}

// Limpeza e encerramento do ImGui:
void EndOperation(GLFWwindow* window) {
    // Fecha a porta serial com segurança
    if (hSerial != INVALID_HANDLE_VALUE && hSerial != NULL) {
        CloseHandle(hSerial);
    }

    // Encerra a interface gráfica
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

// Converte uma imagem de OpenCV (Mat) em uma textura OpenGL (GLuint)
GLuint UpdateGLTexture(GLuint& textureID, const Mat& mat) {
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
    if (textureID == 0) {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    } else {
        glBindTexture(GL_TEXTURE_2D, textureID);
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imagemConvertida.cols, imagemConvertida.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, imagemConvertida.ptr());

    return textureID;
}

int main() {

    // Variáveis
    bool estadoCamera = false;
    bool estadoCameraAnterior = false;
    bool estadoEsteira = false;
    bool Referencia = false;
    bool estadoLuz = false;

    double tempoOperacao = 0.0;
    double tempoJanela = glfwGetTime();
    int total_V = 0;
    int total_O = 0;
    int numContornos = 0;

    Estados _estado = PREPARACAO;
    Estados _estadoAnterior = PREPARACAO;
    
    // Inicializa a janela
    GLFWwindow* window = StartWindow(800, 600, "Sistema de filtragem de vidro para reciclagem");
    if (!window) return -1;

    startImGui(window);

    //Inicializa a comunicação serial
    InicializarSerial("COM1", BAUDRATE);
    if (!InicializarSerial("COM1", BAUDRATE)) {
        cerr << "Falha ao abrir a porta serial!" << endl;
        // return -1; // Não abrirá o programa se não conseguir abrir a porta serial, mas o programa ainda funciona sem a comunicação serial
    } 

    // Chama a imagem da câmera (0 = Padrão notebook)
    VideoCapture cap(0, CAP_DSHOW);
    if (!cap.isOpened()) {
        cerr << "Erro ao abrir a câmera!" << endl;
    }
    GLuint textureCamera = 0; // ID da textura - começa em 0
    GLuint textureReferencia = 0; // ID da referência - começa em 0
    GLuint textureDiff = 0; // ID da subtração - começa em 0
    GLuint textureThresh = 0; // ID da limiarização - começa em 0
    GLuint textureMorph = 0; // ID da morfologia - começa em 0
    Mat frame, frameReferencia, frameDiff, frameThresh, frameMorph;

    // Cor de fundo da janela
    ImVec4 clear_color = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);

    // Loop principal de exibição
    while (!glfwWindowShouldClose(window)) {

        //captura de frame de referência
        if (estadoCamera == true && estadoCameraAnterior == false){
            Referencia = true;
        }
        estadoCameraAnterior = estadoCamera;

        // Botão esteira
        double tempoAtual = glfwGetTime();
        if (estadoEsteira == true){ 
            tempoOperacao += (tempoAtual - tempoJanela);

            if (Referencia == true){
                    frameReferencia = frame.clone();
                    UpdateGLTexture(textureReferencia, frameReferencia);
                    Referencia = false;
                }
        }
        tempoJanela = tempoAtual;

        // Botão câmera
        if (estadoCamera == true){
            if (cap.isOpened()){
                cap >> frame;
            }
        }
    
        Visao(_estado, _estadoAnterior, estadoCamera, estadoLuz, estadoEsteira, frame, frameReferencia, frameDiff, frameThresh, frameMorph, numContornos, total_V, total_O);

        if (estadoCamera == true && (!frame.empty())){
            UpdateGLTexture(textureCamera, frame);
        }

        if (estadoEsteira == true && !frameDiff.empty() && !frameThresh.empty() && !frameMorph.empty()) {
            UpdateGLTexture(textureDiff, frameDiff);
            UpdateGLTexture(textureThresh, frameThresh);
            UpdateGLTexture(textureMorph, frameMorph);
        }

        // Processa eventos do GLFW
        glfwPollEvents();

        // Inicia um novo frame do ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        NewFrame();

        // Desenha a interface do ImGui
        desenharInterface(_estado, _estadoAnterior, textureCamera, textureReferencia, textureDiff, textureThresh, textureMorph, numContornos, estadoEsteira, estadoCamera, estadoCameraAnterior, estadoLuz, tempoOperacao, total_V, total_O);

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

    //Encerra a câmera
    if (cap.isOpened())  cap.release();

    EndOperation(window);

    return 0;
}
