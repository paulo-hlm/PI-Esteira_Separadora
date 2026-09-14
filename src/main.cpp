#include <iostream>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace cv;

// teste att github

int main() {
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