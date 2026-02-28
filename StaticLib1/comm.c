// comm.c
#include "comm.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

// Error handling macro
#define CHECK_ERROR(condition, message) \
    if (condition) { \
        fprintf(stderr, "%s: %d\n", message, GetLastError()); \
        return 1; \
    }

// Socket Communication Functions
int StartSocketSender(const char* message) {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    CHECK_ERROR(iResult != 0, "WSAStartup failed");

    SOCKET ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERROR(ConnectSocket == INVALID_SOCKET, "socket failed");

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(12345);
    iResult = inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    CHECK_ERROR(iResult != 1, "inet_pton failed");

    iResult = connect(ConnectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    CHECK_ERROR(iResult == SOCKET_ERROR, "connect failed");

    iResult = send(ConnectSocket, message, (int)strlen(message) + 1, 0);
    CHECK_ERROR(iResult == SOCKET_ERROR, "send failed");

    printf("Bytes Sent: %d\n", iResult);

    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

int StartSocketReceiver() {
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    CHECK_ERROR(iResult != 0, "WSAStartup failed");

    SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERROR(ListenSocket == INVALID_SOCKET, "socket failed");

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    iResult = bind(ListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    CHECK_ERROR(iResult == SOCKET_ERROR, "bind failed");

    iResult = listen(ListenSocket, SOMAXCONN);
    CHECK_ERROR(iResult == SOCKET_ERROR, "listen failed");

    SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
    CHECK_ERROR(ClientSocket == INVALID_SOCKET, "accept failed");
    closesocket(ListenSocket);

    char recvbuf[1024];
    int recvbuflen = 1024;
    iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0) {
        printf("Bytes received: %d\n", iResult);
        printf("Message: %s\n", recvbuf);
    }
    else if (iResult == 0) {
        printf("Connection closing...\n");
    }
    else {
        CHECK_ERROR(1, "recv failed");
    }

    closesocket(ClientSocket);
    WSACleanup();
    return 0;
}

// Shared Memory Communication Functions
int StartSharedMemorySender(const char* message) {
    HANDLE hMutex = CreateMutex(NULL, FALSE, L"MySharedMemoryMutex");
    CHECK_ERROR(hMutex == NULL, "CreateMutex failed");

    HANDLE hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, L"MySharedMemory");
    CHECK_ERROR(hMapFile == NULL, "CreateFileMapping failed");

    LPVOID lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
    CHECK_ERROR(lpBase == NULL, "MapViewOfFile failed");

    size_t len = (strlen(message) < 1023) ? strlen(message) : 1023;
    memcpy(lpBase, message, len);
    ((char*)lpBase)[len] = '\0';

    ReleaseMutex(hMutex);
    UnmapViewOfFile(lpBase);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    return 0;
}

int StartSharedMemoryReceiver() {
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"MySharedMemoryMutex");
    CHECK_ERROR(hMutex == NULL, "OpenMutex failed");

    HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"MySharedMemory");
    CHECK_ERROR(hMapFile == NULL, "OpenFileMapping failed");

    LPVOID lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
    CHECK_ERROR(lpBase == NULL, "MapViewOfFile failed");

    WaitForSingleObject(hMutex, INFINITE);

    printf("Received message: %s\n", (char*)lpBase);

    ReleaseMutex(hMutex);
    UnmapViewOfFile(lpBase);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    return 0;
}

// Pipe Communication Functions
int StartPipeSender(const char* message) {
    HANDLE hPipe = CreateFile(L"\\\\.\\pipe\\MyPipe", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    CHECK_ERROR(hPipe == INVALID_HANDLE_VALUE, "CreateFile failed");

    DWORD bytesWritten;
    BOOL success = WriteFile(hPipe, message, (DWORD)strlen(message) + 1, &bytesWritten, NULL);
    CHECK_ERROR(!success, "WriteFile failed");

    printf("Message sent successfully.\n");

    CloseHandle(hPipe);
    return 0;
}

// Mailslot Communication Functions
int StartMailslotSender(const char* message) {
    HANDLE hQueue = CreateFile(L"\\\\.\\mailslot\\MyMailSlot", GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    CHECK_ERROR(hQueue == INVALID_HANDLE_VALUE, "CreateFile (Mailslot Sender) failed");

    DWORD bytesWritten;
    BOOL success = WriteFile(hQueue, message, (DWORD)strlen(message) + 1, &bytesWritten, NULL);
    CHECK_ERROR(!success, "WriteFile (Mailslot Sender) failed");

    printf("Message sent successfully. %d bytes written.\n", bytesWritten);

    CloseHandle(hQueue);
    return 0;
}

int StartMailslotReceiver() {
    HANDLE hMailSlot = CreateMailslot(L"\\\\.\\mailslot\\MyMailSlot", 0, MAILSLOT_WAIT_FOREVER, NULL);
    CHECK_ERROR(hMailSlot == INVALID_HANDLE_VALUE, "CreateMailslot failed");

    char buffer[512];
    DWORD bytesRead;

    while (1) { // Keep receiving messages
        BOOL success = ReadFile(hMailSlot, buffer, sizeof(buffer) - 1, &bytesRead, NULL); // -1 to ensure null termination safety

        if (success) {
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0'; // Null-terminate the string
                printf("Received message: %s\n", buffer);
            }
        }
        else {
            DWORD lastError = GetLastError();
            if (lastError == ERROR_MORE_DATA) {
                fprintf(stderr, "Mailslot message too large for buffer.\n");
                // Handle the large message appropriately (e.g., allocate a larger buffer)
            }
            else if (lastError == ERROR_BROKEN_PIPE) {
                printf("No more data in mailslot. Exiting receiver\n");
                break; // exit loop when no more data
            }
            else {
                fprintf(stderr, "ReadFile (Mailslot Receiver) failed: %d\n", lastError);
                break; // Exit the loop on other errors
            }
        }
    }

    CloseHandle(hMailSlot);
    return 0;
}

// Shared Memory Communication Functions
int StartSharedMemorySender(const char* message) {
    HANDLE hMutex = CreateMutex(NULL, FALSE, L"MySharedMemoryMutex");
    CHECK_ERROR(hMutex == NULL, "CreateMutex failed");

    HANDLE hMapFile = CreateFileMapping(
        INVALID_HANDLE_VALUE,    // Use paging file
        NULL,                    // Default security
        PAGE_READWRITE,          // Read/write access
        0,                       // Max. object size: high DWORD
        1024,                    // Max. object size: low DWORD
        L"MySharedMemory");       // Name of mapping object
    CHECK_ERROR(hMapFile == NULL, "CreateFileMapping failed");

    LPVOID lpBase = MapViewOfFile(
        hMapFile,                // Handle to mapping object
        FILE_MAP_ALL_ACCESS,     // Read/write access
        0,                       // High-order byte of file offset
        0,                       // Low-order byte of file offset
        1024);                   // Number of bytes to map
    CHECK_ERROR(lpBase == NULL, "MapViewOfFile failed");

    WaitForSingleObject(hMutex, INFINITE); // Acquire mutex

    size_t len = strlen(message);
    if (len > 1023) {
        len = 1023; // Truncate if message is too long
        fprintf(stderr, "Warning: Message truncated to 1023 characters.\n");
    }
    memcpy(lpBase, message, len);
    ((char*)lpBase)[len] = '\0'; // Null-terminate

    ReleaseMutex(hMutex); // Release mutex

    UnmapViewOfFile(lpBase);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    return 0;
}

int StartSharedMemoryReceiver() {
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, L"MySharedMemoryMutex");
    CHECK_ERROR(hMutex == NULL, "OpenMutex failed");

    HANDLE hMapFile = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // Read/write access
        FALSE,                 // Do not inherit handle
        L"MySharedMemory");     // Name of mapping object
    CHECK_ERROR(hMapFile == NULL, "OpenFileMapping failed");

    LPVOID lpBase = MapViewOfFile(
        hMapFile,              // Handle to mapping object
        FILE_MAP_ALL_ACCESS,   // Read/write access
        0,                     // High-order byte of file offset
        0,                     // Low-order byte of file offset
        1024);                 // Number of bytes to map
    CHECK_ERROR(lpBase == NULL, "MapViewOfFile failed");

    WaitForSingleObject(hMutex, INFINITE); // Acquire mutex

    printf("Received message: %s\n", (char*)lpBase);

    ReleaseMutex(hMutex); // Release mutex

    UnmapViewOfFile(lpBase);
    CloseHandle(hMapFile);
    CloseHandle(hMutex);
    return 0;
}