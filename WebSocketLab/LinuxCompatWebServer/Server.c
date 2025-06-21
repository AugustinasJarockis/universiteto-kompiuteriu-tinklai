#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef linux
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#define CONNECTION_CLOSE(x) close(x)
#endif //linux

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define CONNECTION_CLOSE(x) closesocket(x)
#endif // _Win32

char hostasIPv4[] = "127.0.0.1";
char hostasIPv6[] = "::1";

char portas[] = "20000";

char* ToUpper(char* data);
void GetErrorMsg(wchar_t** buffer);
int ErrorCheck(int error_code, char *msg);

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

    struct addrinfo hints, * res;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, portas, &hints, &res)) != 0) {
        printf("getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    int listening_socket = ErrorCheck(socket(res->ai_family, res->ai_socktype, res->ai_protocol), "Socket problem");

    int optVal = 0;
    ErrorCheck(setsockopt(listening_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&optVal, sizeof(optVal)), "Disabling IPV6_ONLY problem");

    ErrorCheck(bind(listening_socket, res->ai_addr, res->ai_addrlen), "Binding problem");

    ErrorCheck(listen(listening_socket, 10), "Ipv6 Listening problem");

    printf("Serveris veikia!\n");
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);

    int new_socket_handle = ErrorCheck(accept(listening_socket, (struct sockaddr*)&their_addr, &addr_size), "New socket problem");
    
    while (1) {
        char duomenys[256] = { 0 };
        int read_val = recv(new_socket_handle, &duomenys, 256, 0);
        if (read_val == -1) {
            break;
        }
        printf("Serveris gavo: %s\n", duomenys);
        char* did_duomenys = ToUpper(duomenys);
        int len = strlen(did_duomenys);
        send(new_socket_handle, did_duomenys, len + 1, 0);
        free(did_duomenys);
    }

    CONNECTION_CLOSE(listening_socket);
    CONNECTION_CLOSE(new_socket_handle);
    freeaddrinfo(res); // free the linked list
#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
    getc(stdin);
    return 0;
}

char* ToUpper(char* data)
{
    char* big_data = malloc(1024);
    int i;
    for (i = 0; data[i] != 0; i++)
    {
        if (data[i] <= 'z' && data[i] >= 'a') {
            big_data[i] = data[i] - 32;
        }
        else {
            big_data[i] = data[i];
        }
    }
    big_data[i] = '\0';
    return big_data;
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
