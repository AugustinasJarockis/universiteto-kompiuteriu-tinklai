#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>

int ErrorCheck(int error_code, char* msg);

//char hostas[] = "127.0.0.1";
char hostas[] = "::1";
char portas[] = "20000";

int main(int argc, char* argv[]) {
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

    struct addrinfo hints, * res, * p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostas, portas, &hints, &res)) != 0) {
        fprintf(stdout, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    int sending_socket = ErrorCheck(socket(res->ai_family, res->ai_socktype, res->ai_protocol), "Socket problem");

    ErrorCheck(connect(sending_socket, res->ai_addr, res->ai_addrlen), "Connection problem");

    printf("Vesk eilut siuntimui ... --> \n");

    char input[256] = { 0 };
    fgets(input, 256, stdin);
    input[strlen(input) - 1] = 0;

    send(sending_socket, input, strlen(input), 0);

    char atsakymas[256];
    recv(sending_socket, atsakymas, 256, 0);

    printf("Gavome: %s\n", atsakymas);

    closesocket(sending_socket);

    freeaddrinfo(res); // free the linked list
    WSACleanup();
    getc(stdin);
    return 0;
}

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
