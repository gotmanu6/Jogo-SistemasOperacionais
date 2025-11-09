#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <winsock2.h>
#include <windows.h> 

WSADATA winsocketsDados;
SOCKET sock_listen;
struct sockaddr_in server_addr;

// Sockets dos Jogadores
SOCKET player1_sock = INVALID_SOCKET;
SOCKET player2_sock = INVALID_SOCKET;

typedef struct {
    SOCKET sock;
    int id; // 1 ou 2
    char nick[32];
} ClientInfo;

ClientInfo clients[2];


int iniciarBiblioteca();
int iniciarSocket();
void finalizaConexao();

DWORD WINAPI ClientHandler(LPVOID lpParam);

// Funcao para enviar uma mensagem
void SendMessageToClient(SOCKET s, const char* msg);

// Funcao para receber uma mensagem
int ReceiveMessage(SOCKET s, char* buffer, int bufferSize);


// FUNCOES DE SETUP 

int iniciarBiblioteca() {
    if (WSAStartup(MAKEWORD(2, 2), &winsocketsDados) != 0) {
        printf("Falha ao inicializar o Winsock\n");
        return 1;
    }
    printf("WSAStartup carregado com sucesso\n");
    return 0;
}

int iniciarSocket() {
    sock_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_listen == INVALID_SOCKET) {
        printf("Erro ao criar o socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    printf("Socket criado com sucesso\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(51171); // A mesma porta do cliente
    
    if (bind(sock_listen, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Erro ao associar o socket: %d\n", WSAGetLastError());
        closesocket(sock_listen);
        WSACleanup();
        return 1;
    }
    printf("Bind realizado com sucesso\n");
    return 0;
}

//LOGICA DE I/O

void SendMessageToClient(SOCKET s, const char* msg) {
    if (s != INVALID_SOCKET) {
        send(s, msg, strlen(msg), 0);
    }
}

int ReceiveMessage(SOCKET s, char* buffer, int bufferSize) {
    int bytesReceived = recv(s, buffer, bufferSize - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        return bytesReceived;
    } else if (bytesReceived == 0) {
        return 0; // 
    } else {
        return -1;
    }
}


// --- FUNCAO DA THREAD DE COMUNICACAO ---


DWORD WINAPI ClientHandler(LPVOID lpParam) {
    ClientInfo* info = (ClientInfo*)lpParam;
    
    char welcome_msg[64];
    sprintf(welcome_msg, "Voce e o Jogador %d. Aguardando o outro jogador.\n", info->id);
    SendMessageToClient(info->sock, welcome_msg);
    return 0; 
}






int main() {
    if (iniciarBiblioteca() != 0 || iniciarSocket() != 0) {
        return 1;
    }

    if (listen(sock_listen, SOMAXCONN) == SOCKET_ERROR) {
        printf("Erro ao colocar o socket em estado de escuta: %d\n", WSAGetLastError());
        closesocket(sock_listen);
        WSACleanup();
        return 1;
    }
    printf("Servidor em escuta na porta 51171. Esperando 2 clientes...\n");

    // CONEXAO DOS 2 CLIENTES
    
    int players_connected = 0;
while (players_connected < 2) {
    SOCKET new_sock;
    struct sockaddr_in client_addr;
    int clientAddrLen = sizeof(client_addr);

    // ACEITA A CONEXAO (Bloqueia ate um cliente tentar conectar)
    new_sock = accept(sock_listen, (struct sockaddr*)&client_addr, &clientAddrLen);
    if (new_sock == INVALID_SOCKET) {
        printf("Erro ao aceitar a conexao: %d\n", WSAGetLastError());
        continue;
    }
    
    char nick_buffer[32];
    
    // ADD NICK
    
    if (player1_sock == INVALID_SOCKET) {
        // Atribui o socket e ID temporariamente
        player1_sock = new_sock;
        clients[0].sock = new_sock;
        clients[0].id = 1;

        // PROTOCOLO: Pede e recebe o nickname do Jogador 1
        SendMessageToClient(clients[0].sock, "CLIENTE 1. Digite seu NICKNAME: ");
        
        if (ReceiveMessage(clients[0].sock, nick_buffer, sizeof(nick_buffer)) > 0) {
            // Armazena e confirma
            strncpy(clients[0].nick, nick_buffer, 31);
            clients[0].nick[31] = '\0';
            printf("Cliente 1 aceito. Nickname: %s\n", clients[0].nick);
            SendMessageToClient(clients[0].sock, "OK. Aguardando Cliente 2...");
            
            
            players_connected++; 
        } else {
            // Falhou em receber o nick 
            printf("Cliente 1 desconectou durante o nickname.\n");
            closesocket(new_sock);
            player1_sock = INVALID_SOCKET; // Reseta para tentar novamente
            continue;
        }
        
    } else if (player2_sock == INVALID_SOCKET) {
        // Atribui o socket e ID temporariamente
        player2_sock = new_sock;
        clients[1].sock = new_sock;
        clients[1].id = 2;

        // PROTOCOLO: Pede e recebe o nickname do Jogador 2
        SendMessageToClient(clients[1].sock, "CLIENTE 2. Digite seu NICKNAME: ");
        
        if (ReceiveMessage(clients[1].sock, nick_buffer, sizeof(nick_buffer)) > 0) {
            // Armazena e confirma
            strncpy(clients[1].nick, nick_buffer, 31);
            clients[1].nick[31] = '\0';
            printf("Cliente 2 aceito. Nickname: %s. Jogo PRONTO!\n", clients[1].nick);
            
            // Envia mensagem de inicio para AMBOS os jogadores
            char start_msg[128];
            sprintf(start_msg, "Bem-vindos, %s e %s! Jogo iniciado.", clients[0].nick, clients[1].nick);
            SendMessageToClient(clients[0].sock, start_msg);
            SendMessageToClient(clients[1].sock, start_msg);

          
            players_connected++;
        } else {
            // Falhou em receber o nick
            printf("Cliente 2 desconectou durante o nickname.\n");
            closesocket(new_sock);
            player2_sock = INVALID_SOCKET;
            continue;
        }
    }
    
	}
	// Fecha o socket de escuta, pois nao aceitaremos mais clientes
		closesocket(sock_listen);



    // TROCA DE MENSAGENS SIMPLES 
    printf("\n--- INICIANDO CHAT DE TESTE (Alternado) ---\n");
    char buffer[512];
    
    while (1) {
        // Recebe do Cliente 1
        SendMessageToClient(player1_sock, "Sua vez (Enviar Msg p/ J2): ");
        if (ReceiveMessage(player1_sock, buffer, sizeof(buffer)) <= 0) break;
        printf("%s (1) diz: %s\n", clients[0].nick, buffer);

        // Retransmite para Cliente 2
        char send_to_j2[512];
        sprintf(send_to_j2, "%s (j1) diz: %s", clients[0].nick, buffer);
        SendMessageToClient(player2_sock, send_to_j2);


        // Recebe do Cliente 2
        SendMessageToClient(player2_sock, "Sua vez (Enviar Msg p/ J1): ");
        if (ReceiveMessage(player2_sock, buffer, sizeof(buffer)) <= 0) break;
        printf("%s (j2) diz: %s\n", clients[1].nick, buffer);

        // Retransmite para Cliente 1
        char send_to_j1[512];
        sprintf(send_to_j1, "%s (2) diz: %s", clients[1].nick, buffer);
        SendMessageToClient(player1_sock, send_to_j1);
    }


    // --- FINALIZACAO ---
    printf("\nEncerrando conexoes.\n");
    closesocket(player1_sock);
    closesocket(player2_sock);
    WSACleanup();
    return 0;
}
