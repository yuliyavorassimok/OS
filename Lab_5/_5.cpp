#include <iostream>
#include <conio.h>
#include <fstream>
#include <time.h>
#include <algorithm>
#include <process.h>
#include <windows.h>
#include "employee.h"

int empCount;
employee* emps;
HANDLE* hReadyEvents;
CRITICAL_SECTION empsCS;
bool* empIsModifying;
const char pipeName[30] = "\\\\.\\pipe\\pipe_name";

void sortEmps() {
    qsort(emps, empCount, sizeof(employee), empCmp);
}

void writeData(char filename[50]) {
    std::fstream fin(filename, std::ios::binary | std::ios::out);
    fin.write(reinterpret_cast<char*>(emps), sizeof(employee) * empCount);
    fin.close();
}

void readDataSTD() {
    emps = new employee[empCount];
    std::cout << "Enter ID, name and working hours of each employee:\n";
    for (int i = 1; i <= empCount; ++i) {
        std::cout << i << ":\n";
        std::cin >> emps[i - 1].num >> emps[i - 1].name >> emps[i - 1].hours;
    }
}

employee* findEmp(int id) {
    employee key;
    key.num = id;
    return (employee*)bsearch((const char*)(&key), (const char*)(emps), empCount, sizeof(employee), empCmp);
}

void startPocesses(int count) {
    char buff[10];
    for (int i = 0; i < count; i++) {
        char cmdargs[80] = "client.exe ";
        char eventName[50] = "READY_EVENT_";
        _itoa_s(i + 1, buff, 10);
        strcat_s(eventName, buff);
        strcat_s(cmdargs, eventName);
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        hReadyEvents[i] = CreateEvent(nullptr, TRUE, FALSE, eventName);
        if (!CreateProcess(nullptr, cmdargs, nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE,
            nullptr, nullptr, &si, &pi)) {
            printf("Creation process error.\n");
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
}

DWORD WINAPI messaging(LPVOID p) {
    HANDLE hPipe = p;
    //getting emp with id -1 means for client that error occurred
    auto* errorEmp = new employee;
    errorEmp->num = -1;
    while (true) {
        DWORD readBytes;
        char message[10];
        //receiving a message
        bool isRead = ReadFile(hPipe, message, 10, &readBytes, nullptr);
        if (!isRead) {
            if (ERROR_BROKEN_PIPE == GetLastError()) {
                std::cout << "Client disconnected." << std::endl;
                break;
            }
            else {
                std::cerr << "Error in reading a message." << std::endl;
                break;
            }
        }
        //sending answer
        if (strlen(message) > 0) {
            char command = message[0];
            message[0] = ' ';
            int id = atoi(message);
            DWORD bytesWritten;
            EnterCriticalSection(&empsCS);
            employee* empToSend = findEmp(id);
            LeaveCriticalSection(&empsCS);
            if (nullptr == empToSend) {
                empToSend = errorEmp;
            }
            else {
                __int64 ind = empToSend - emps;
                if (empIsModifying[ind])
                    empToSend = errorEmp;
                else {
                    switch (command) {
                    case 'w':
                        printf("Requested to modify ID %d.", id);
                        empIsModifying[ind] = true;
                        break;
                    case 'r':
                        printf("Requested to read ID %d.", id);
                        break;
                    default:
                        std::cout << "Unknown request. ";
                        empToSend = errorEmp;
                    }
                }
            }
            bool isSent = WriteFile(hPipe, empToSend, sizeof(employee), &bytesWritten, nullptr);
            if (isSent)
                std::cout << "Answer is sent." << std::endl;
            else
                std::cout << "Error in sending answer." << std::endl;
            //receiving a changed record
            if ('w' == command && empToSend != errorEmp) {
                isRead = ReadFile(hPipe, empToSend, sizeof(employee), &readBytes, nullptr);
                if (isRead) {
                    std::cout << "Employee record changed." << std::endl;
                    empIsModifying[empToSend - emps] = false;
                    EnterCriticalSection(&empsCS);
                    sortEmps();
                    LeaveCriticalSection(&empsCS);
                }
                else {
                    std::cerr << "Error in reading a message." << std::endl;
                    break;
                }
            }
        }
    }
    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);
    delete errorEmp;
    return 0;
}

void openPipes(int clientCount) {
    HANDLE hPipe;
    HANDLE* hThreads = new HANDLE[clientCount];
    for (int i = 0; i < clientCount; i++) {
        hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, nullptr);
        if (INVALID_HANDLE_VALUE == hPipe) {
            std::cerr << "Create named pipe failed." << std::endl;
            _getch();
            return;
        }
        if (!ConnectNamedPipe(hPipe, nullptr)) {
            std::cout << "No connected clients." << std::endl;
            break;
        }
        hThreads[i] = CreateThread(nullptr, 0, messaging, (LPVOID)hPipe, 0, nullptr);
    }
    std::cout << "Clients connected to pipe." << std::endl;
    WaitForMultipleObjects(clientCount, hThreads, TRUE, INFINITE);
    std::cout << "All clients are disconnected." << std::endl;
    delete[] hThreads;
}

int main() {
    //data input
    char filename[50];
    std::cout << "Enter the file name and the count of employees. \n";
    std::cin >> filename >> empCount;
    readDataSTD();
    writeData(filename);
    sortEmps();

    //creating processes
    InitializeCriticalSection(&empsCS);
    srand(static_cast<unsigned int>(time(nullptr)));
    int clientCount = 2 + rand() % 3; //from 2 to 4
    HANDLE hstartALL = CreateEvent(nullptr, TRUE, FALSE, "START_ALL");
    empIsModifying = new bool[empCount];
    for (int i = 0; i < empCount; ++i)
        empIsModifying[i] = false;
    hReadyEvents = new HANDLE[clientCount];
    startPocesses(clientCount);
    WaitForMultipleObjects(clientCount, hReadyEvents, TRUE, INFINITE);
    std::cout << "All processes are ready.Starting." << std::endl;
    SetEvent(hstartALL);

    //creating pipes
    openPipes(clientCount);
    for (int i = 0; i < empCount; i++)
        emps[i].print(std::cout);
    std::cout << "Press any key to exit" << std::endl;
    _getch();
    DeleteCriticalSection(&empsCS);
    delete[] empIsModifying;
    delete[] hReadyEvents;
    delete[] emps;
    return 0;
}
