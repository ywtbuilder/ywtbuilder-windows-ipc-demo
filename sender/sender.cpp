// sender.cpp
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <string>
#include <limits>
#include "comm.h" // Include your comm.h

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "comm.lib") // Link with your comm.lib

int main() {
    while (true) {
        int choice;
        std::cout << "Select Sender communication method:\n";
        std::cout << "1. Socket\n";
        std::cout << "2. Shared Memory\n";
        std::cout << "3. Pipe\n";
        std::cout << "4. Mailslot\n";
        std::cout << "5. Exit\n";
        std::cout << "Enter your choice: ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
            std::cerr << "Invalid input. Please enter a number from 1 to 5.\n";
            continue;
        }
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

        if (choice == 5) {
            std::cout << "Exiting sender program.\n";
            break;
        }

        std::string message;
        std::cout << "Enter message: ";
        std::getline(std::cin, message);

        int result = 0;
        switch (choice) {
        case 1:
            result = StartSocketSender(message.c_str());
            break;
        case 2:
            result = StartSharedMemorySender(message.c_str());
            break;
        case 3:
            result = StartPipeSender(message.c_str());
            break;
        case 4:
            result = StartMailslotSender(message.c_str());
            break;
        default:
            std::cerr << "Invalid choice. Please try again.\n";
            continue;
        }

        if (result != 0) {
            std::cerr << "Sender operation failed.\n";
        }

        system("pause");
    }

    return 0;
}

