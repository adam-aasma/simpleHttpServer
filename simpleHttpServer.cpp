// simpleHttpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <string>

#define DEFAULT_PORT "8088"
bool initWinsock(WSAData& wsaData);
SOCKET acceptClient(SOCKET listenSocket);
bool startListen(SOCKET listenSocket);
SOCKET createListenSocket();
std::string readData(SOCKET clientSocket);
void sendData(const char* data, SOCKET clientSocket);

int main()
{
    WSADATA wsaData;
    if (initWinsock(wsaData)) {
        SOCKET listenSocket = createListenSocket();
        if (listenSocket != INVALID_SOCKET) {
            if (startListen(listenSocket)) {
                SOCKET clientSocket = acceptClient(listenSocket);
                std::string data = readData(clientSocket);
                printf(data.c_str());
                sendData("Thank you\r\n", clientSocket);
            }
            closesocket(listenSocket);
        }
    }
    printf("Bye bye!\n");

    WSACleanup();
}

void sendData(const char* data, SOCKET clientSocket)
{
    send(clientSocket, data, strlen(data), 0);
}

std::string readData(SOCKET clientSocket)
{
#define DEFAULT_BUFLEN 512

    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;
    int iOffs = 0;
    recvbuf[0] = 0;
    do {
        int iResult = recv(clientSocket, recvbuf + iOffs, recvbuflen - iOffs, 0);
        if (iResult > 0) {
            iOffs += iResult;
            recvbuf[iOffs] = 0;
        }
    } while (!strstr(recvbuf, "\n"));

    std::string s(recvbuf, iOffs);

    return s;
}

bool initWinsock(WSAData& wsaData)
{
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return false;
    }
    return true;
}

SOCKET acceptClient(SOCKET listenSocket)
{
    // Accept a client socket
    SOCKET clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        //closesocket(ListenSocket);
        //WSACleanup();
        return INVALID_SOCKET;
    }
    return clientSocket;
}

bool startListen(SOCKET listenSocket)
{
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        //closesocket(listenSocket);
        //WSACleanup();
        return false;
    }
    return true;
}

SOCKET createListenSocket()
{
    struct addrinfo hints; 
    struct addrinfo* result = NULL;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    int iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        //WSACleanup();
        return INVALID_SOCKET;
    }

    SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    // Setup the TCP listening socket
    iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(listenSocket);
        //WSACleanup();
        return INVALID_SOCKET;
    }

    freeaddrinfo(result);
    return listenSocket;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
