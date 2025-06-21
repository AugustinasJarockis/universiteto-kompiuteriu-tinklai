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

int ErrorCheck(int error_code, char* msg);

//char hostas[] = "127.0.0.1";
char hostas[] = "::1";
//char hostas[] = "localhost";
char portas[20] = "20000";

#ifdef linux
void* UserInputListener(void* arg) {
#endif // linux
#ifdef _WIN32
DWORD WINAPI ClientInputListener(LPVOID arg) {
#endif // _WIN32
    char input[256] = { 0 };
    int sending_socket = *(int*)arg;

    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    while (1) {
        fgets(input, 256, stdin);
        input[strlen(input) - 1] = 0;
        if (strcmp(input, "BAIGTI") == 0) {
            CONNECTION_CLOSE(sending_socket);
            break;
        }
        send(sending_socket, input, strlen(input), 0);
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
    printf("Vesk serverio porta: \n");
    scanf("%s", &portas);
    printf("Priimtas portas: %s\n", portas);

    struct addrinfo hints, * res, * p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostas, portas, &hints, &res)) != 0) {
        fprintf(stdout, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    int* sending_socket = (int*)malloc(sizeof(int));
    *sending_socket = ErrorCheck(socket(res->ai_family, res->ai_socktype, res->ai_protocol), "Socket problem");

    ErrorCheck(connect(*sending_socket, res->ai_addr, res->ai_addrlen), "Connection problem");

#ifdef linux
    pthread_t tid;
    int error_val = pthread_create(&tid, NULL, UserInputListener, (void*)sending_socket);
    if (error_val != NULL) {
        fprintf(stderr, "Could not create Thread\n");
        exit(4);
    }
#endif // linux

#ifdef _WIN32
    HANDLE handle = CreateThread(NULL, 0, ClientInputListener, sending_socket, 0, NULL);
    if (handle == NULL) {
        fprintf(stderr, "Could not create Thread\n");
        exit(4);
    }
#endif // _WIN32

    while (1) {
        char atsakymas[256] = { 0 };
        int read_val = recv(*sending_socket, atsakymas, 256, 0);
        if (read_val == -1) {
            break;
        }
        printf("[Serveris]: %s\n", atsakymas);
    }

    CONNECTION_CLOSE(*sending_socket);

    freeaddrinfo(res); // free the linked list
    free(sending_socket);
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
