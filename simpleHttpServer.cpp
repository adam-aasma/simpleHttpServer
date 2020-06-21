// simpleHttpServer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <string>

#define DEFAULT_PORT "8088"
#define DEFAULT_BUFLEN 512

struct SocketData {
    SOCKET socket;
    char recvbuf[DEFAULT_BUFLEN];
    int recvBufOffs = 0;
};

bool initWinsock(WSAData& wsaData);
SOCKET acceptClient(SOCKET listenSocket);
bool startListen(SOCKET listenSocket);
SOCKET createListenSocket();
std::string readData(SOCKET clientSocket);
void sendData(const char* data, SOCKET clientSocket);
void handleBuffer(SocketData* pSocketData, int);
bool isBufferComplete(SocketData* pSocketData);
void resetBuffer(SocketData* pSocketData);

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

int readData(SOCKET clientSocket, char* recvBuf, int recvBufLen)
{
    return recv(clientSocket, recvBuf, recvBufLen, 0);
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
        return INVALID_SOCKET;
    }
    return clientSocket;
}

bool startListen(SOCKET listenSocket)
{
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        return false;
    }
    SocketData sockets[64];
    int noOfSockets = 0;
    sockets[noOfSockets].socket = listenSocket;
    noOfSockets++;
    fd_set readfds;
    timeval timeOut;
    timeOut.tv_sec = 10;
    while (true) {
        FD_ZERO(&readfds);
        for (int i = 0; i < noOfSockets; i++) {
            FD_SET(sockets[i].socket, &readfds);
        }
        int noOfSet = select(0, &readfds, NULL, NULL, &timeOut);
        if (noOfSet == SOCKET_ERROR) {
            printf("Select failed with error: %ld\n", WSAGetLastError());
            return false;
        }
        if (noOfSet > 0) {
            for (int i = 0; i < noOfSockets; i++) {
                if (FD_ISSET(sockets[i].socket, &readfds)) {
                    if (i == 0) {
                        SOCKET newClient = acceptClient(listenSocket);
                        sockets[noOfSockets].socket = newClient;
                        sockets[noOfSockets].recvBufOffs = 0;
                        noOfSockets++;
                    }
                    else {
                        SocketData* s = sockets + i;
                        int iResult = readData(s->socket, s->recvbuf + s->recvBufOffs, DEFAULT_BUFLEN - s->recvBufOffs);
                        if (!iResult) {
                            if (i + 1 < noOfSockets) {
                                int byteSize = sizeof(SocketData);
                                void* dest = ((char*)sockets) + i * byteSize;
                                void* src = ((char*)sockets) + (i + 1) * byteSize;
                                size_t totSize = (noOfSockets - (i + 1)) * sizeof(SocketData);

                                memmove(dest, src, totSize);
                            }
                            noOfSockets--;
                        }
                        else {
                            handleBuffer(s, iResult);
                            if (isBufferComplete(s)) {
                                for (int j = 1; j < noOfSockets; j++) {
                                    sendData(s->recvbuf, sockets[j].socket);
                                }
                                resetBuffer(s);
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

void handleBuffer(SocketData* pSocketData, int dataRead)
{
    pSocketData->recvBufOffs += dataRead;
    pSocketData->recvbuf[pSocketData->recvBufOffs] = 0;
}

bool isBufferComplete(SocketData* pSocketData)
{
    return strstr(pSocketData->recvbuf, "\n");
}

void resetBuffer(SocketData* pSocketData)
{
    pSocketData->recvBufOffs = 0;
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
