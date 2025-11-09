#include <winsock2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// --- VARIÁVEIS GLOBAIS DE CONEXÃO ---
WSADATA winsocketsDados;
SOCKET clientSocket;
struct sockaddr_in serverAddr;

// Buffers de I/O
char sendBuffer[512];
char recvBuffer[512];


// --- FUNÇÕES DE SETUP (Mínimas alterações) ---

int iniciarBiblioteca(){
    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0) {
        printf("Falha ao inicializar o Winsock\n");
        return 1;
    } else {
        printf("WSAStartup carregado com sucesso\n");
        return 0;
    }
}

int iniciarClientSocket(){
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        printf("Erro ao criar o socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    } else {
        printf("Socket criado com sucesso\n");
        return 0;
    }
}

void configurarEnderecoServidor() { // Nome da função alterado para clareza
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Loopback para o mesmo PC
    serverAddr.sin_port = htons(51171);
}

int iniciarConexao() {
    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Erro ao conectar ao servidor: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    } else {
        printf("Conectado ao servidor\n");
        return 1;
    }
}

// --- FUNÇÕES DE COMUNICAÇÃO (Limpos de I/O de teclado) ---

// Envia a string fornecida
int enviarMensagem(const char* msg){
    int bytesSent = send(clientSocket, msg, strlen(msg), 0);
    if (bytesSent == SOCKET_ERROR) {
        printf("Erro ao enviar dados: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    }
    return 0;
}

// Recebe a mensagem e retorna o buffer preenchido ou um status
int receberMensagem(char* buffer, int bufferSize) {
    int bytesReceived = recv(clientSocket, buffer, bufferSize - 1, 0);
    if (bytesReceived == SOCKET_ERROR) {
        printf("Erro ao receber dados: %d\n", WSAGetLastError());
        closesocket(clientSocket);
        WSACleanup();
        return -1;
    } else if (bytesReceived == 0) {
        printf("Conexao fechada pelo servidor\n");
        return -1;
    } else {
        buffer[bytesReceived] = '\0';
        return 1;
    }
}

void finalizaConexao(){
    closesocket(clientSocket);
    WSACleanup();
    // Não é necessário getch() se o terminal estiver aberto após a execução
}


// --- FUNÇÃO PRINCIPAL (Focada no fluxo de chat) ---

int main() {
    // 1. INICIALIZAÇÃO
    if (iniciarBiblioteca() != 0) return 1;
    if (iniciarClientSocket() != 0) {
        WSACleanup();
        return 1;
    }
    configurarEnderecoServidor();
    if (iniciarConexao() == -1) {
        return 1;
    }

    // Recebe a primeira mensagem (Identificação de J1/J2)
    if (receberMensagem(recvBuffer, sizeof(recvBuffer)) == 1) {
        printf("Servidor: %s\n", recvBuffer);
    }
    
  // --- 2. PROTOCOLO DE NICKNAME (NOVA SEÇÃO) ---

    // A) Recebe a instrução do servidor (Ex: "CLIENTE 1. Digite seu NICKNAME: ")
    if (receberMensagem(recvBuffer, sizeof(recvBuffer)) == -1) { 
        finalizaConexao(); 
        return 1;
    }
    printf("Servidor: %s\n", recvBuffer);

    // B) Pega o nickname do teclado
    printf("Meu nickname: ");
    fgets(sendBuffer, sizeof(sendBuffer), stdin);
    sendBuffer[strcspn(sendBuffer, "\n")] = 0; // Remove newline

    // C) Envia o nickname
    if (enviarMensagem(sendBuffer) == -1) {
        finalizaConexao();
        return 1;
    }

    // D) Recebe a confirmação/status do servidor (Ex: "Bem-vindo, [Nick]! Aguarde...")
    if (receberMensagem(recvBuffer, sizeof(recvBuffer)) == -1) { 
        finalizaConexao(); 
        return 1;
    }
    printf("Servidor: %s\n", recvBuffer);
    
    
    // --- 3. LOOP PRINCIPAL DE COMUNICAÇÃO (O seu antigo 'while(1)') ---
    printf("\n--- INICIO DO CHAT/JOGO ---\n");
    
    while (1) {
        // ... (Seu código de loop de chat/jogo continua aqui) ...

        // A) Espera a instrução do servidor para jogar/falar
        if (receberMensagem(recvBuffer, sizeof(recvBuffer)) == -1) {
            break;
   		}
        printf("\nServidor diz: %s", recvBuffer); // Imprime a instrução

        // B) Pega a entrada do usuário
        printf("Minha mensagem: ");
        fgets(sendBuffer, sizeof(sendBuffer), stdin);
        sendBuffer[strcspn(sendBuffer, "\n")] = 0; // Remove newline
        
        // C) Envia a mensagem
        if (enviarMensagem(sendBuffer) == -1) {
            break;
        }
        
        if (strcmp(sendBuffer, "exit") == 0) {
            printf("Encerrando a conexao...\n");
            break;
        }

        // D) Espera a resposta/confirmação do servidor (Ex: "J1 diz: ...")
        if (receberMensagem(recvBuffer, sizeof(recvBuffer)) == -1) {
            break;
        }
        printf("Servidor: %s\n", recvBuffer);
    }

    // 3. FINALIZAÇÃO
    finalizaConexao();
    return 0;
}

