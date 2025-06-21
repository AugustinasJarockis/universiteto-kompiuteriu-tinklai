#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef linux
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>

#define CONNECTION_CLOSE(x) close(x)
#endif // linux

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#define CONNECTION_CLOSE(x) closesocket(x)
#endif // _WIN32

struct KlientoServerioSoketuPora {
    int kliento_soketas;
    int serverio_soketas;
};

int ErrorCheck(int error_code, char* msg);

//char hostas[] = "127.0.0.1";
char hostas[] = "::1";
//char hostas[] = "localhost";
char kliento_portas[20] = "20000";
char serverio_portas[20] = "40000";

#ifdef linux
void* ClientInputListener(void* arg) {
#endif // linux
#ifdef _WIN32
    DWORD WINAPI ClientInputListener(LPVOID arg) {
#endif // _WIN32
        struct KlientoServerioSoketuPora pora = *(struct KlientoServerioSoketuPora*)arg;
        int client_socket = pora.kliento_soketas;
        int server_socket = pora.serverio_soketas;

        while (1) {
            char atsakymas[256] = { 0 };
            int read_val = recv(client_socket, atsakymas, 256, 0);
            if (read_val == -1) {
                break;
            }
            send(server_socket, atsakymas, strlen(atsakymas), 0);
            printf("[Klientas]: %s\n", atsakymas);
        }
        return NULL;
    }

    int main(int argc, char* argv[]) {
#ifdef _WIN32
        WSADATA wsaData;

        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            fprintf(stderr, "WSAStartup failed.\n");
            exit(1);
        }
        if (LOBYTE(wsaData.wVersion) != 2 ||
            HIBYTE(wsaData.wVersion) != 2)
        {
            fprintf(stderr, "Versiion 2.2 of Winsock is not available.\n");
            WSACleanup();
            exit(2);
        }
#endif // _WIN32
        printf("Vesk kliento porta: \n");
        scanf("%s", &kliento_portas);
        printf("Priimtas portas: %s\n", kliento_portas);

        printf("Vesk kito serverio porta: \n");
        scanf("%s", &serverio_portas);
        printf("Priimtas portas: %s\n", serverio_portas);

        struct addrinfo hints, hints2, *res, *res2;
        int status;

        memset(&hints, 0, sizeof hints);
        memset(&hints2, 0, sizeof hints2);
        hints.ai_family = AF_INET6; // AF_INET or AF_INET6 to force version
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE;

        hints2.ai_family = AF_INET6; // AF_INET or AF_INET6 to force version
        hints2.ai_socktype = SOCK_STREAM;
        hints2.ai_flags = AI_PASSIVE;

        if ((status = getaddrinfo(NULL, kliento_portas, &hints, &res)) != 0) {
            fprintf(stdout, "getaddrinfo: %s\n", gai_strerror(status));
            return 2;
        }

        if ((status = getaddrinfo(hostas, serverio_portas, &hints2, &res2)) != 0) {
            fprintf(stdout, "getaddrinfo: %s\n", gai_strerror(status));
            return 2;
        }

        int client_socket = ErrorCheck(socket(res->ai_family, res->ai_socktype, res->ai_protocol), "Socket problem");
        int optVal = 0;
        ErrorCheck(setsockopt(client_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&optVal, sizeof(optVal)), "Disabling IPV6_ONLY problem");
        ErrorCheck(bind(client_socket, res->ai_addr, res->ai_addrlen), "Binding problem");
        ErrorCheck(listen(client_socket, 10), "Ipv6 Listening problem");

        int server_socket = ErrorCheck(socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol), "Server socket problem");
        int result = connect(server_socket, res2->ai_addr, res2->ai_addrlen);
        
        int isToListen = 0;
        int new_server_socket = server_socket;
        struct sockaddr_storage their_addr;
        socklen_t addr_size = sizeof(their_addr);
        if (result == -1) {
            server_socket = ErrorCheck(socket(res2->ai_family, res2->ai_socktype, res2->ai_protocol), "Recreating server socket problem");
            int optVal2 = 0;
            ErrorCheck(setsockopt(server_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&optVal2, sizeof(optVal)), "Disabling IPV6_ONLY problem");
            ErrorCheck(bind(server_socket, res2->ai_addr, res2->ai_addrlen), "Binding problem");
            ErrorCheck(listen(server_socket, 10), "Ipv6 Listening problem");
            isToListen = 1;
            new_server_socket = ErrorCheck(accept(server_socket, (struct sockaddr*)&their_addr, &addr_size), "New socket problem");
        }

        printf("Serveris veikia\n");

        addr_size = sizeof(their_addr);
        int new_client_socket = ErrorCheck(accept(client_socket, (struct sockaddr*)&their_addr, &addr_size), "New client socket problem");

        struct KlientoServerioSoketuPora *pora = malloc(sizeof(struct KlientoServerioSoketuPora));
        (*pora).kliento_soketas = new_client_socket;
        (*pora).serverio_soketas = new_server_socket;
#ifdef linux
        pthread_t tid;
        int error_val = pthread_create(&tid, NULL, ClientInputListener, (void*)pora);
        if (error_val != NULL) {
            fprintf(stderr, "Could not create Thread\n");
            exit(4);
        }
#endif // linux

#ifdef _WIN32
        HANDLE handle = CreateThread(NULL, 0, ClientInputListener, pora, 0, NULL);
        if (handle == NULL) {
            fprintf(stderr, "Could not create Thread\n");
            exit(4);
        }
#endif // _WIN32
        while (1) {
            char atsakymas[256] = { 0 };
            int read_val = recv(new_server_socket, atsakymas, 256, 0);
            if (read_val == -1) {
                break;
            }
            send(new_client_socket, atsakymas, strlen(atsakymas), 0);
            printf("[Serveris]: %s\n", atsakymas);
        }
        printf("Serveris baigia dirbti\n");

        CONNECTION_CLOSE(client_socket);
        CONNECTION_CLOSE(server_socket);
        CONNECTION_CLOSE(new_server_socket);
        CONNECTION_CLOSE(new_client_socket);

        freeaddrinfo(res); // free the linked list
        freeaddrinfo(res2); // free the linked list
        free(pora);
#ifdef _WIN32
        WSACleanup();
#endif // _WIN32
        getc(stdin);
        return 0;
    }

#ifdef _WIN32
    void GetErrorMsg(wchar_t** buffer) {
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, WSAGetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPWSTR)buffer, 0, NULL);
    }

    int ErrorCheck(int error_code, char* msg) {
        if (error_code == -1) {
            wchar_t* buffer = NULL;
            GetErrorMsg(&buffer);
            printf("%s: %ls\n", msg, buffer);
            LocalFree(buffer);
            WSACleanup();
            fflush(stdin);
            getc(stdin);
            exit(3);
        }
        return error_code;
    }
#endif // _WIN32

#ifdef linux
    int ErrorCheck(int error_code, char* msg) {
        if (error_code == -1) {
            printf("%s: %s\n", msg, strerror(errno));
            getc(stdin);
            exit(3);
        }
        return error_code;
    }
#endif // linux
