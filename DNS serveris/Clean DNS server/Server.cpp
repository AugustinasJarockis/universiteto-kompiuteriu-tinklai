#define _CRT_SECURE_NO_WARNINGS

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <sstream>

#ifdef linux
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SOCKET_ERROR -1
#define CONNECTION_CLOSE(x) close(x)
#define THREAD_RETURN_TYPE void*
#endif // linux


#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>

#define CONNECTION_CLOSE(x) closesocket(x)
#define THREAD_RETURN_TYPE DWORD WINAPI
std::string hosts_file_path = "C:\\Windows\\System32\\drivers\\etc\\hosts";
#endif // _WIN32


char hostasIPv4[] = "127.0.0.1";
//char hostasIPv4[] = "10.254.3.174";
//char hostasIPv4[] = "192.168.127.127";
//char hostasIPv6[] = "::1";

char googleDNS[] = "8.8.4.4";

char portas[] = "53";
int int_portas = 53;

std::map<std::string, std::string> domain_name_table;

int google_soketas;

void PrintQueryInfo(char query[], int query_len);

int CreateGoogleSocket(sockaddr_in& google_server);
int CreateListeningSocket();

bool IsIpAddress(std::string potential_ip);
bool SameClientIpAddress(char ClientIP[], char hostasIPv4[]);

std::string GenerateQuestionField(const std::string &name, unsigned short question_type, unsigned short question_class);

int ErrorCheck(int error_code, const char* msg);
#ifdef _WIN32
void GetErrorMsg(wchar_t** buffer);
void InitialiseWSA();
#endif // _WIN32

int main(int argc, char* argv[]) {
#ifdef _WIN32
    InitialiseWSA();
#endif // _WIN32
    std::string domain_name;
    std::string ip_address;
    
    std::ifstream hosts_input(hosts_file_path);
    while (!hosts_input.eof()) {
        char hosts_buffer[512] = {};
        hosts_input.getline(hosts_buffer, 512);
        std::stringstream stream(hosts_buffer);
        stream >> ip_address >> domain_name;
        if (ip_address[0] != '#') {
            domain_name_table.insert({ domain_name, ip_address });
        }
    }
    hosts_input.close();

    std::ifstream fd("NameFile.dns");
    
    while (!fd.eof()) {
        fd >> domain_name >> ip_address;
        domain_name_table.insert({ domain_name, ip_address });
    }
    fd.close();

    //Google DNS =================================
    
    sockaddr_in google_server;
    
    google_soketas = CreateGoogleSocket(google_server);
    //Google DNS ---------------------------------

    sockaddr_in server{};

    // prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    inet_pton(AF_INET, hostasIPv4, (struct sockaddr_in*)&server.sin_addr);
    server.sin_port = htons(int_portas);

    //int listening_socket = ErrorCheck(socket(AF_INET6, SOCK_DGRAM, 0), "Socket problem");
    int listening_socket = ErrorCheck(socket(AF_INET, SOCK_DGRAM, 0), "Socket problem");
    //int optVal = 0;
    //ErrorCheck(setsockopt(listening_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&optVal, sizeof(optVal)), "Disabling IPV6_ONLY problem");
    ErrorCheck(bind(listening_socket, (sockaddr*)&server, sizeof(server)), "Binding problem");

    printf("Serveris veikia!\n");

    while (true) {
        sockaddr_in client{};

        char message[512] = {};

        // try to receive some data, this is a blocking call
        int message_len;
        socklen_t slen = sizeof(sockaddr_in);
        if ((message_len = recvfrom(listening_socket, message, 512, 0, (sockaddr*)&client, &slen)) == SOCKET_ERROR) {
#ifdef _WIN32
            std::cout << "Client recvfrom() failed with error code: " << WSAGetLastError() << "\n";
#endif // _WIN32
#ifdef linux
            std::cout << "Client recvfrom() failed with error code: " << strerror(errno) << "\n";
#endif // linux
            CONNECTION_CLOSE(listening_socket);
            listening_socket = CreateListeningSocket();
            std::cout << "Listening socket was recreated" << std::endl;
            continue;
        }

        //Printing of received thing =============================================================
        unsigned short header_id = ((unsigned short)message[0] << 8) + (unsigned short)message[1];
        short QD_COUNT = ((short)message[4] << 8) + (short)message[5];
        short AN_COUNT = ((short)message[6] << 8) + (short)message[7];
        short NS_COUNT = ((short)message[8] << 8) + (short)message[9];
        short AR_COUNT = ((short)message[10] << 8) + (short)message[11];

        std::cout << std::endl << "Gauta uzklausa: ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ " << std::endl;
        std::cout << "Header id: " << header_id << std::endl;
        std::cout << (((message[3] >> 7) == 1) ? "Query response" : "Query") << std::endl;
        std::cout << "QDCOUNT: " << QD_COUNT << std::endl;
        std::cout << "ANCOUNT: " << AN_COUNT << std::endl;
        std::cout << "NSCOUNT: " << NS_COUNT << std::endl;
        std::cout << "ARCOUNT: " << AR_COUNT << std::endl;

        std::string request_header(message, 12);
        int index = 12;
        std::string complete_name = "";
        for (int i = 0; i < QD_COUNT; i++) {
            std::cout << "QNAME: ";
            for (; message[index] != 0; index++) {
                std::cout << (int)message[index] << " ";
                int oct_size = (int)message[index] + index;
                for (index++; index <= oct_size; index++) {
                    std::cout << message[index];
                    complete_name.push_back(message[index]);
                }
                index--;
                std::cout << " ";
                complete_name += '.';
            }
            complete_name.pop_back();
            std::cout << std::endl;
            index++;
            std::cout << "Complete name: " << complete_name << std::endl;
            std::cout << "QTYPE: " << ((short)message[index] << 8) + (short)message[index + 1] << std::endl;
            std::cout << "QCLASS: " << ((short)message[index + 2] << 8) + (short)message[index + 3] << std::endl;
        }
        for (int i = 0; i < complete_name.size(); i++) {
            if (complete_name[i] <= 'Z' && complete_name[i] >= 'A') {
                complete_name[i] += 32;
            }
        }
        int question_size = index + 3 - 12 + 1;

        std::cout << std::endl << "All message: " << std::endl;
        for (int i = 0; i < message_len; i++) {
            std::cout << (int)message[i] << " ";
        }
        std::cout << std::endl;

        //Ignore HTTPS QTYPE questions
        if (((short)message[index] << 8) + (short)message[index + 1] == 65) {
            std::cout << "It is HTTPS query. This server ignores these" << std::endl;
            continue;
        }

        std::cout << "Generating response" << std::endl;
        char answer[512] = {};
        int answer_length;
        std::string ip_address;

        char message_to_google[512] = {};
        int message_to_google_len;

        bool IsCNAME = false;
        
        std::string original_name;
        if (domain_name_table.find(complete_name) != domain_name_table.end() && !IsIpAddress(domain_name_table[complete_name])) {
            original_name = complete_name;
            
            while (domain_name_table.find(complete_name) != domain_name_table.end() && !IsIpAddress(domain_name_table[complete_name])) {
                std::cout << "It is not an IP address" << std::endl;
                complete_name = domain_name_table[complete_name];
                std::cout << "Looking for name: " << complete_name << std::endl;
            }

            std::string question_field = GenerateQuestionField(complete_name, 1, 1);
            std::string query_to_be_sent = "";
            query_to_be_sent += request_header;
            query_to_be_sent += question_field;

            query_to_be_sent.copy(message_to_google, query_to_be_sent.size(), 0);
            message_to_google_len = query_to_be_sent.size();

            IsCNAME = true;
        }
        else {
            memcpy(message_to_google, message, 512);
            message_to_google_len = message_len;
        }
        
        if (complete_name == "localhost") {
            char bin_client[4] = {};
            bin_client[0] = client.sin_addr.S_un.S_un_b.s_b1;
            bin_client[1] = client.sin_addr.S_un.S_un_b.s_b2;
            bin_client[2] = client.sin_addr.S_un.S_un_b.s_b3;
            bin_client[3] = client.sin_addr.S_un.S_un_b.s_b4;
            
            if (!SameClientIpAddress(bin_client, hostasIPv4)) {
                continue;
            }
        }

        if (domain_name_table.find(complete_name) == domain_name_table.end()) {
            //Google stuff =========================================================================================================
            std::cout << "Sending request to Google" << std::endl;
            ErrorCheck(sendto(google_soketas, message_to_google, message_to_google_len, 0, (sockaddr*)&google_server, sizeof(sockaddr_in)), "sendto() failed with error code: ");


            // try to receive some data, this is a blocking call
            socklen_t slen2 = sizeof(sockaddr_in);
            if ((answer_length = recvfrom(google_soketas, answer, 512, 0, (sockaddr*)&google_server, &slen2)) == SOCKET_ERROR) {
#ifdef _WIN32
                std::cout << "recvfrom() failed with error code: " << WSAGetLastError() << "\n";
#endif // _WIN32
#ifdef linux
                std::cout << "recvfrom() failed with error code: " << strerror(errno) << "\n";
#endif // linux

                CONNECTION_CLOSE(google_soketas);
                google_soketas = CreateGoogleSocket(google_server);
                std::cout << "Socket was recreated" << std::endl;
                continue;
            }
            //Google end ----------------------------------------------------------------------------------------------------------
            std::cout << std::endl << "GOOGLE RESPONSE INFO: ================" << std::endl;
            PrintQueryInfo(answer, answer_length);
            std::cout << std::endl << "GOOGLE RESPONSE INFO END: ================" << std::endl << std::endl;
            if (IsCNAME) {
                std::string response_header(answer, 12);
                std::string response_question_field = GenerateQuestionField(original_name, 1, 1);
                std::string response_to_be_sent = "";
                response_to_be_sent += response_header;
                response_to_be_sent += response_question_field;
                
                int index = 12;
                for (int i = 0; i < QD_COUNT; i++) {
                   
                    for (; message[index] != 0; index++) {
                        int oct_size = (int)message[index] + index;
                        for (index++; index <= oct_size; index++) {
                        }
                        index--;
                    }
                    index++;
                }

                std::string answer_part(answer + index - 4, answer + answer_length);

                response_to_be_sent += answer_part;

                for (int i = 0; i < response_to_be_sent.size(); i++) {
                    answer[i] = response_to_be_sent[i];
                }
                answer_length = response_to_be_sent.size();
                std::cout << "Printing ------------------------------------" << std::endl;
                PrintQueryInfo(answer, answer_length);
                std::cout << "Good" << std::endl;
            }
        }
        else {
            ip_address = domain_name_table[complete_name];

            std::cout << "Domain name is known for the server" << std::endl;

            std::string dns_response = "";
            
            dns_response += message[0];
            dns_response += message[1];
            dns_response += (char)0b10000000;
            dns_response += (char)0b00000000;
            dns_response += (char)0;
            dns_response += (char)1; //QDCOUNT
            dns_response += (char)0;
            dns_response += (char)1; //Answer count
            dns_response += (char)0;
            dns_response += (char)0; //NSCOUNT
            dns_response += (char)0;
            dns_response += (char)0; //ARCOUNT

            //Question repeat
            for (int i = 0; i < question_size; i++) {
                dns_response += message[i + 12];
            }
            dns_response += 0b11000000; //Pointer to name = c00c (12)
            dns_response += 0b00001100; //Pointer to name = c00c (12)
            dns_response += (char)0; //TYPE
            dns_response += (char)1; //TYPE
            dns_response += (char)0; //CLASS
            dns_response += (char)1; //CLASS
            dns_response += (char)0; //TTL
            dns_response += (char)0; //TTL
            dns_response += (char)0; //TTL
            dns_response += (char)0; //TTL

            dns_response += (char)0; //RDLENGTH - response ipv4 is 4 bytes long
            dns_response += (char)4; //RDLENGTH - response ipv4 is 4 bytes long

            int ip_index = 0;
            for (int i = 0; i < 4; i++) {
                std::string ip_octet = "";
                while (ip_address[ip_index] != '.' && ip_index < ip_address.length()) {
                    ip_octet += ip_address[ip_index];
                    ip_index++;
                }
                ip_index++;
                dns_response += (char)stoi(ip_octet);
                std::cout << " " << (unsigned int)stoi(ip_octet) << std::endl;
            }

            answer_length = dns_response.length();

            for (int i = 0; i < answer_length; i++) {
                answer[i] = dns_response[i];
            }

            std::cout << std::endl << "RESPONSE INFO: ================" << std::endl;
            PrintQueryInfo(answer, answer_length);
            std::cout << std::endl << "RESPONSE INFO END: ================" << std::endl << std::endl;
        }

        std::cout << std::endl << "Siunciamas atskaymas" << std::endl;
        ErrorCheck(sendto(listening_socket, answer, answer_length, 0, (sockaddr*)&client, sizeof(sockaddr_in6)), "sendto() failed with error code: ");
    }
    
    CONNECTION_CLOSE(listening_socket);
#ifdef _WIN32
    WSACleanup();
#endif // _WIN32
    return 0;
}

std::string GenerateQuestionField(const std::string& name, unsigned short question_type, unsigned short question_class) {
    std::string result = "";
    
    //Add name
    std::string name_part = "";
    for (int i = 0; i < name.size(); i++) {
        if (name[i] == '.') {
            result += (char)name_part.size();
            result += name_part;
            name_part = "";
            continue;
        }
        name_part += name[i];
    }
    result += (char)name_part.size();
    result += name_part;
    result += (char)0;

    //Add QTYPE
    result += (char)((question_type & 0b1111111100000000) >> 8);
    result += (char)(question_type & 0b11111111);

    //Add QCLASS
    result += (char)((question_class & 0b1111111100000000) >> 8);
    result += (char)(question_class & 0b11111111);

    return result;
}

bool IsIpAddress(std::string potential_ip) {
    if (potential_ip.size() > 15 || potential_ip.size() < 7)
        return false;

    std::string octet = "";
    int index = 0;
    for (int i = 0; i < 4 && index < potential_ip.size(); index++, i++) {
        while (potential_ip[index] != '.' && index < potential_ip.size()) {
            if (!(potential_ip[index] >= '0' && potential_ip[index] <= '9')) {
                return false;
            }
            octet += potential_ip[index];
            index++;
        }
        int int_octet = stoi(octet);
        if (!(int_octet >= 0 && int_octet <= 255)) {
            return false;
        }
        octet = "";
    }
    if (index < potential_ip.size()) {
        return false;
    }
    return true;
}

bool SameClientIpAddress(char ClientIP[], char hostasIPv4[]) {
    int address_size = strlen(hostasIPv4);

    unsigned char bin_host[4] = {};
    std::string octet = "";
    int index = 0;
    for (int i = 0; i < 4 && index < address_size; index++, i++) {
        while (hostasIPv4[index] != '.' && index < address_size) {
            octet += hostasIPv4[index];
            index++;
        }
        int int_octet = stoi(octet);
        bin_host[i] = (unsigned char)int_octet;
        octet = "";
    }

    for (int i = 0; i < 4; i++) {
        if (bin_host[i] != ClientIP[i]) {
            return false;
        }
    }
    return true;
}

void PrintQueryInfo(char query[], int query_len) {
    unsigned short header_id = ((unsigned short)query[0] << 8) + (unsigned short)query[1];
    short QD_COUNT = ((short)query[4] << 8) + (short)query[5];
    short AN_COUNT = ((short)query[6] << 8) + (short)query[7];
    short NS_COUNT = ((short)query[8] << 8) + (short)query[9];
    short AR_COUNT = ((short)query[10] << 8) + (short)query[11];

    std::cout << "Here: " << std::endl;
    std::cout << "Header id: " << header_id << std::endl;
    std::cout << (((query[3] >> 7) == 1) ? "Query response" : "Query") << std::endl;
    std::cout << "QDCOUNT: " << QD_COUNT << std::endl;
    std::cout << "ANCOUNT: " << AN_COUNT << std::endl;
    std::cout << "NSCOUNT: " << NS_COUNT << std::endl;
    std::cout << "ARCOUNT: " << AR_COUNT << std::endl;

    int index = 12;
    std::string complete_name = "";
    for (int i = 0; i < QD_COUNT; i++) {
        std::cout << "QNAME: ";
        for (; query[index] != 0; index++) {
            std::cout << (int)query[index] << " ";
            int oct_size = (int)query[index] + index;
            for (index++; index <= oct_size; index++) {
                std::cout << query[index];
                complete_name.push_back(query[index]);
            }
            index--;
            std::cout << " ";
            complete_name += '.';
        }
        complete_name.pop_back();
        std::cout << std::endl;
        index++;
        std::cout << "Complete name: " << complete_name << std::endl;
        std::cout << "QTYPE: " << ((short)query[index] << 8) + (short)query[index + 1] << std::endl;
        std::cout << "QCLASS: " << ((short)query[index + 2] << 8) + (short)query[index + 3] << std::endl;
    }
    int question_size = index + 3 - 12 + 1;

    std::cout << std::endl << "All message: " << std::endl;
    for (int i = 0; i < query_len; i++) {
        std::cout << (int)query[i] << " ";
    }
    std::cout << std::endl;
}

int CreateListeningSocket() {
    sockaddr_in server{};

    server.sin_family = AF_INET;
    inet_pton(AF_INET, hostasIPv4, (struct sockaddr_in*)&server.sin_addr);
    server.sin_port = htons(int_portas);

    //int listening_socket = ErrorCheck(socket(AF_INET6, SOCK_DGRAM, 0), "Socket problem");
    int listening_socket = ErrorCheck(socket(AF_INET, SOCK_DGRAM, 0), "Socket problem");
    //int optVal = 0;
    //ErrorCheck(setsockopt(listening_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&optVal, sizeof(optVal)), "Disabling IPV6_ONLY problem");
    ErrorCheck(bind(listening_socket, (sockaddr*)&server, sizeof(server)), "Binding problem");

    return listening_socket;
}

int CreateGoogleSocket(sockaddr_in& google_server) {

    int google_soketas = ErrorCheck(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP), "Google socket creation problem");

    memset((char*)&google_server, 0, sizeof(google_server));
    google_server.sin_family = AF_INET;
    google_server.sin_port = htons(53);

#ifdef _WIN32
    google_server.sin_addr.S_un.S_addr = 8 + 8 * 256 + 4 * 256 * 256 + 4 * 256 * 256 * 256;
#endif // _WIN32
#ifdef linux
    google_server.sin_addr.s_addr = 8 + 8 * 256 + 4 * 256 * 256 + 4 * 256 * 256 * 256;
#endif // linux

    return google_soketas;
}

#ifdef linux
int ErrorCheck(int error_code, const char* msg) {
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

int ErrorCheck(int error_code, const char* msg) {
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