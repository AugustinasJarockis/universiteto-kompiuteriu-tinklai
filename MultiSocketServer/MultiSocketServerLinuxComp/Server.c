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

#define TRUE 1
#define FALSE 0
#endif // linux


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#define CONNECTION_CLOSE(x) closesocket(x)
#endif // _WIN32


char hostasIPv4[] = "127.0.0.1";
char hostasIPv6[] = "::1";

char portas[] = "20000";

#ifdef _WIN32
HANDLE lockOnVardai;
#endif // _WIN32
#ifdef linux
pthread_mutex_t lockOnVardai;
#endif // linux

char vardai[1000][256] = { 0 };
int soketai[1000] = { 0 };
int dabartinis_serveriu_sk = 0;

int ErrorCheck(int error_code, char* msg);
#ifdef _WIN32
void GetErrorMsg(wchar_t** buffer);
void InitialiseWSA();
#endif // _WIN32
int YraVardas(const char* vardas);
int Pasalink(const char* vardas);
void IstrinkVarda(int index);
void IstrinkSoketa(int index);

#ifdef linux
void* Valdymas(void* lpArg)
#endif // linux
#ifdef _WIN32
DWORD WINAPI Valdymas(LPVOID lpArg)
#endif // _WIN32
{
    char vardas[256] = { 0 };
    int socket_handle;
#ifdef _WIN32
    __try {
#endif //_WIN32
        socket_handle = *(int*)lpArg;
        while (1) {
            char buffer[16] = "ATSIUSKVARDA\n";
            send(socket_handle, buffer, strlen(buffer), 0);
            recv(socket_handle, vardas, 256, 0);
            if (vardas[0] == NULL) {
                return NULL;
            }
            vardas[strlen(vardas) - 1] = 0;
            if (vardas[strlen(vardas) - 1] == '\r') {
                vardas[strlen(vardas) - 1] = 0;
            }
#ifdef linux
            pthread_mutex_lock(&lockOnVardai);
            if (!YraVardas(vardas)) {
                strcpy(vardai[dabartinis_vartotoju_sk], vardas);
                soketai[dabartinis_vartotoju_sk] = socket_handle;
                dabartinis_vartotoju_sk++;
                pthread_mutex_unlock(&lockOnVardai);
                break;
            }
            pthread_mutex_unlock(&lockOnVardai);
#endif // linux

#ifdef _WIN32
            DWORD wait_result = WaitForSingleObject(lockOnVardai, INFINITE);

            switch (wait_result)
            {
                case WAIT_OBJECT_0:
                    __try {
                        if (!YraVardas(vardas)) {
                            strcpy(vardai[dabartinis_serveriu_sk], vardas);
                            soketai[dabartinis_serveriu_sk] = socket_handle;
                            dabartinis_serveriu_sk++;
                            goto APPROVE_NAME;
                        }
                    }

                    __finally {
                        // Release ownership of the mutex object
                        if (!ReleaseMutex(lockOnVardai))
                        {
                            printf("Problem releasing the lock\n");
                        }
                    }
                    break;

                case WAIT_ABANDONED:
                    return FALSE;
            }
#endif //_WIN32
        }

    APPROVE_NAME:
        send(socket_handle, "VARDASOK\n", strlen("VARDASOK\n"), 0);

        while (1) {
            char duomenys[512] = { 0 };
            int read_val = recv(socket_handle, &duomenys, 512, 0);
            if (read_val == -1) {
                break;
            }

            for (int i = 0; i < dabartinis_serveriu_sk; i++) {
                char message[1024] = "PRANESIMAS";
                strcat(message, vardas);
                strcat(message, ": ");
                strcat(message, duomenys);
                send(soketai[i], message, strlen(message), 0);
            }
        }
#ifdef _WIN32
    }
    __finally {
#endif // _WIN32
        if (vardas[0] != 0) {
            int indeksas = Pasalink(vardas);
            IstrinkSoketa(indeksas);
            dabartinis_serveriu_sk--;
        }
        CONNECTION_CLOSE(socket_handle);
        free(lpArg);
#ifdef _WIN32
    }
#endif // _WIN32
    return NULL;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    InitialiseWSA();

    lockOnVardai = CreateMutex(NULL, FALSE, NULL);
    if (lockOnVardai == NULL)
    {
        printf("CreateMutex error: %d\n", GetLastError());
        return 1;
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

#ifdef _WIN32
    __try {
#endif // _WIN32
        while (1) {
            int* new_socket_handle = (int*)malloc(sizeof(int));
            struct sockaddr_storage their_addr;
            socklen_t addr_size = sizeof(their_addr);
            *new_socket_handle = ErrorCheck(accept(listening_socket, (struct sockaddr*)&their_addr, &addr_size), "New socket problem");
#ifdef linux
            pthread_t tid;
            int error_val = pthread_create(&tid, NULL, Valdymas, (void*)new_socket_handle);
            if (error_val != NULL) {
                fprintf(stderr, "Could not create Thread\n");
            }
#endif // linux

#ifdef _WIN32
            HANDLE handle = CreateThread(NULL, 0, Valdymas, new_socket_handle, 0, NULL);
            if (handle == NULL) {
                fprintf(stderr, "Could not create Thread\n");
            }
#endif // _WIN32
        }
#ifdef _WIN32
    }
    __finally {
#endif // _WIN32
        CONNECTION_CLOSE(listening_socket);
        freeaddrinfo(res); // free the linked list
#ifdef _WIN32
        WSACleanup();
    }
#endif // _WIN32
    return 0;
}

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

void InitialiseWSA() {
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
}
#endif // _WIN32

int YraVardas(const char* vardas) {
    for (int i = 0; i < dabartinis_serveriu_sk; i++) {
        if (strcmp(vardas, vardai[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

int Pasalink(const char* vardas) { //Reikia sinchronizuoti sita
    for (int i = 0; i < dabartinis_serveriu_sk; i++) {
        if (strcmp(vardas, vardai[i]) == 0) {
            IstrinkVarda(i);
            return i;
        }
    }
}

void IstrinkVarda(int index) {
    for (int i = index; i < dabartinis_serveriu_sk - 1; i++) {
        strcpy(vardai[i], vardai[i + 1]);
    }
}

void IstrinkSoketa(int index) {
    for (int i = index; i < dabartinis_serveriu_sk - 1; i++) {
        soketai[i] = soketai[i + 1];
    }
}