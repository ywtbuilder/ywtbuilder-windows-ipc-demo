// comm.c
#include "comm.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define SOCKET_PORT 12345
#define SOCKET_ADDR "127.0.0.1"

#define SHARED_BUFFER_SIZE 1024
#define SHARED_WAIT_MS 15000
#define SHARED_MEM_NAME L"Local\\MySharedMemory"
#define SHARED_MUTEX_NAME L"Local\\MySharedMemoryMutex"
#define SHARED_EVENT_NAME L"Local\\MySharedMemoryReady"

#define PIPE_NAME L"\\\\.\\pipe\\MyPipe"
#define MAILSLOT_NAME L"\\\\.\\mailslot\\MyMailSlot"
#define MAILSLOT_WAIT_MS 15000

static int PrintWin32ErrorCode(const char* message, DWORD code) {
    fprintf(stderr, "%s: %lu\n", message, code);
    return 1;
}

static int PrintWin32Error(const char* message) {
    return PrintWin32ErrorCode(message, GetLastError());
}

static int PrintSocketError(const char* message) {
    fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
    return 1;
}

// Socket Communication Functions
int StartSocketSender(const char* message) {
    int result = 1;
    WSADATA wsaData;
    SOCKET connectSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr;
    int iResult;

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", iResult);
        return 1;
    }

    connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connectSocket == INVALID_SOCKET) {
        result = PrintSocketError("socket failed");
        goto cleanup;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SOCKET_PORT);
    iResult = inet_pton(AF_INET, SOCKET_ADDR, &serverAddr.sin_addr);
    if (iResult != 1) {
        result = PrintSocketError("inet_pton failed");
        goto cleanup;
    }

    iResult = connect(connectSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        result = PrintSocketError("connect failed");
        goto cleanup;
    }

    iResult = send(connectSocket, message, (int)strlen(message) + 1, 0);
    if (iResult == SOCKET_ERROR) {
        result = PrintSocketError("send failed");
        goto cleanup;
    }

    printf("Bytes sent: %d\n", iResult);
    result = 0;

cleanup:
    if (connectSocket != INVALID_SOCKET) {
        closesocket(connectSocket);
    }
    WSACleanup();
    return result;
}

int StartSocketReceiver() {
    int result = 1;
    WSADATA wsaData;
    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;
    struct sockaddr_in serverAddr;
    int iResult;
    char recvbuf[SHARED_BUFFER_SIZE];

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        fprintf(stderr, "WSAStartup failed: %d\n", iResult);
        return 1;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        result = PrintSocketError("socket failed");
        goto cleanup;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(SOCKET_PORT);

    iResult = bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    if (iResult == SOCKET_ERROR) {
        result = PrintSocketError("bind failed");
        goto cleanup;
    }

    iResult = listen(listenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        result = PrintSocketError("listen failed");
        goto cleanup;
    }

    clientSocket = accept(listenSocket, NULL, NULL);
    if (clientSocket == INVALID_SOCKET) {
        result = PrintSocketError("accept failed");
        goto cleanup;
    }

    iResult = recv(clientSocket, recvbuf, (int)sizeof(recvbuf), 0);
    if (iResult > 0) {
        if (iResult >= (int)sizeof(recvbuf)) {
            recvbuf[sizeof(recvbuf) - 1] = '\0';
        }
        else {
            recvbuf[iResult] = '\0';
        }
        printf("Bytes received: %d\n", iResult);
        printf("Message: %s\n", recvbuf);
        result = 0;
    }
    else if (iResult == 0) {
        printf("Connection closing...\n");
        result = 0;
    }
    else {
        result = PrintSocketError("recv failed");
    }

cleanup:
    if (clientSocket != INVALID_SOCKET) {
        closesocket(clientSocket);
    }
    if (listenSocket != INVALID_SOCKET) {
        closesocket(listenSocket);
    }
    WSACleanup();
    return result;
}

// Shared Memory Communication Functions
int StartSharedMemorySender(const char* message) {
    int result = 1;
    HANDLE hMutex = NULL;
    HANDLE hMapFile = NULL;
    HANDLE hEvent = NULL;
    LPVOID lpBase = NULL;
    DWORD waitResult;
    BOOL hasLock = FALSE;
    size_t len;

    hMutex = CreateMutexW(NULL, FALSE, SHARED_MUTEX_NAME);
    if (hMutex == NULL) {
        return PrintWin32Error("CreateMutex failed");
    }

    hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_BUFFER_SIZE, SHARED_MEM_NAME);
    if (hMapFile == NULL) {
        result = PrintWin32Error("CreateFileMapping failed");
        goto cleanup;
    }

    hEvent = CreateEventW(NULL, TRUE, FALSE, SHARED_EVENT_NAME);
    if (hEvent == NULL) {
        result = PrintWin32Error("CreateEvent failed");
        goto cleanup;
    }

    lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_BUFFER_SIZE);
    if (lpBase == NULL) {
        result = PrintWin32Error("MapViewOfFile failed");
        goto cleanup;
    }

    waitResult = WaitForSingleObject(hMutex, SHARED_WAIT_MS);
    if (waitResult != WAIT_OBJECT_0) {
        if (waitResult == WAIT_TIMEOUT) {
            fprintf(stderr, "WaitForSingleObject (mutex) timed out.\n");
        }
        else {
            result = PrintWin32Error("WaitForSingleObject (mutex) failed");
        }
        goto cleanup;
    }
    hasLock = TRUE;

    if (!ResetEvent(hEvent)) {
        result = PrintWin32Error("ResetEvent failed");
        goto cleanup;
    }

    len = strlen(message);
    if (len >= SHARED_BUFFER_SIZE) {
        len = SHARED_BUFFER_SIZE - 1;
    }
    memcpy(lpBase, message, len);
    ((char*)lpBase)[len] = '\0';

    if (!SetEvent(hEvent)) {
        result = PrintWin32Error("SetEvent failed");
        goto cleanup;
    }

    result = 0;

cleanup:
    if (hasLock) {
        ReleaseMutex(hMutex);
    }
    if (lpBase != NULL) {
        UnmapViewOfFile(lpBase);
    }
    if (hEvent != NULL) {
        CloseHandle(hEvent);
    }
    if (hMapFile != NULL) {
        CloseHandle(hMapFile);
    }
    if (hMutex != NULL) {
        CloseHandle(hMutex);
    }
    return result;
}

int StartSharedMemoryReceiver() {
    int result = 1;
    HANDLE hMutex = NULL;
    HANDLE hMapFile = NULL;
    HANDLE hEvent = NULL;
    LPVOID lpBase = NULL;
    DWORD waitResult;
    BOOL hasLock = FALSE;

    hMutex = CreateMutexW(NULL, FALSE, SHARED_MUTEX_NAME);
    if (hMutex == NULL) {
        return PrintWin32Error("CreateMutex failed");
    }

    hMapFile = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_BUFFER_SIZE, SHARED_MEM_NAME);
    if (hMapFile == NULL) {
        result = PrintWin32Error("CreateFileMapping failed");
        goto cleanup;
    }

    hEvent = CreateEventW(NULL, TRUE, FALSE, SHARED_EVENT_NAME);
    if (hEvent == NULL) {
        result = PrintWin32Error("CreateEvent failed");
        goto cleanup;
    }

    lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_BUFFER_SIZE);
    if (lpBase == NULL) {
        result = PrintWin32Error("MapViewOfFile failed");
        goto cleanup;
    }

    waitResult = WaitForSingleObject(hEvent, SHARED_WAIT_MS);
    if (waitResult != WAIT_OBJECT_0) {
        if (waitResult == WAIT_TIMEOUT) {
            fprintf(stderr, "Shared memory read timed out. Start sender and retry.\n");
        }
        else {
            result = PrintWin32Error("WaitForSingleObject (event) failed");
        }
        goto cleanup;
    }

    waitResult = WaitForSingleObject(hMutex, SHARED_WAIT_MS);
    if (waitResult != WAIT_OBJECT_0) {
        if (waitResult == WAIT_TIMEOUT) {
            fprintf(stderr, "WaitForSingleObject (mutex) timed out.\n");
        }
        else {
            result = PrintWin32Error("WaitForSingleObject (mutex) failed");
        }
        goto cleanup;
    }
    hasLock = TRUE;

    printf("Received message: %s\n", (char*)lpBase);

    if (!ResetEvent(hEvent)) {
        result = PrintWin32Error("ResetEvent failed");
        goto cleanup;
    }

    result = 0;

cleanup:
    if (hasLock) {
        ReleaseMutex(hMutex);
    }
    if (lpBase != NULL) {
        UnmapViewOfFile(lpBase);
    }
    if (hEvent != NULL) {
        CloseHandle(hEvent);
    }
    if (hMapFile != NULL) {
        CloseHandle(hMapFile);
    }
    if (hMutex != NULL) {
        CloseHandle(hMutex);
    }
    return result;
}

// Pipe Communication Functions
int StartPipeSender(const char* message) {
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    DWORD bytesWritten;

    if (!WaitNamedPipeW(PIPE_NAME, SHARED_WAIT_MS)) {
        return PrintWin32Error("WaitNamedPipe failed");
    }

    hPipe = CreateFileW(PIPE_NAME, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        return PrintWin32Error("CreateFile failed");
    }

    if (!WriteFile(hPipe, message, (DWORD)strlen(message) + 1, &bytesWritten, NULL)) {
        CloseHandle(hPipe);
        return PrintWin32Error("WriteFile failed");
    }

    printf("Message sent successfully.\n");
    CloseHandle(hPipe);
    return 0;
}

int StartPipeReceiver() {
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    BOOL success;
    DWORD bytesRead = 0;
    char buffer[512];

    hPipe = CreateNamedPipeW(
        PIPE_NAME,
        PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
        1,
        0,
        0,
        0,
        NULL);
    if (hPipe == INVALID_HANDLE_VALUE) {
        return PrintWin32Error("CreateNamedPipe failed");
    }

    success = ConnectNamedPipe(hPipe, NULL);
    if (!success) {
        DWORD lastError = GetLastError();
        if (lastError != ERROR_PIPE_CONNECTED) {
            CloseHandle(hPipe);
            return PrintWin32ErrorCode("ConnectNamedPipe failed", lastError);
        }
    }

    success = ReadFile(hPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
    if (!success) {
        CloseHandle(hPipe);
        return PrintWin32Error("ReadFile failed");
    }

    buffer[bytesRead] = '\0';
    printf("Received message: %s\n", buffer);
    CloseHandle(hPipe);
    return 0;
}

// Mailslot Communication Functions
int StartMailslotSender(const char* message) {
    HANDLE hQueue = CreateFileW(MAILSLOT_NAME, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    DWORD bytesWritten;

    if (hQueue == INVALID_HANDLE_VALUE) {
        return PrintWin32Error("CreateFile (Mailslot Sender) failed");
    }

    if (!WriteFile(hQueue, message, (DWORD)strlen(message) + 1, &bytesWritten, NULL)) {
        CloseHandle(hQueue);
        return PrintWin32Error("WriteFile (Mailslot Sender) failed");
    }

    printf("Message sent successfully. %lu bytes written.\n", bytesWritten);

    CloseHandle(hQueue);
    return 0;
}

int StartMailslotReceiver() {
    HANDLE hMailSlot = INVALID_HANDLE_VALUE;
    char buffer[512];
    DWORD bytesRead = 0;
    BOOL success;

    hMailSlot = CreateMailslotW(MAILSLOT_NAME, 0, MAILSLOT_WAIT_MS, NULL);
    if (hMailSlot == INVALID_HANDLE_VALUE) {
        return PrintWin32Error("CreateMailslot failed");
    }

    success = ReadFile(hMailSlot, buffer, sizeof(buffer) - 1, &bytesRead, NULL);
    if (!success) {
        DWORD lastError = GetLastError();
        CloseHandle(hMailSlot);
        if (lastError == ERROR_SEM_TIMEOUT) {
            fprintf(stderr, "Mailslot receive timed out. Start sender and retry.\n");
            return 1;
        }
        return PrintWin32ErrorCode("ReadFile (Mailslot Receiver) failed", lastError);
    }

    buffer[bytesRead] = '\0';
    printf("Received message: %s\n", buffer);

    CloseHandle(hMailSlot);
    return 0;
}
