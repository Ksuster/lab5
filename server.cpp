#include <iostream>
#include <conio.h>
#include <fstream>
#include <time.h>
#include <algorithm>
#include <process.h>
#include <windows.h>
#include "employee.h"

using namespace std;

const char pipeName[30] = "\\\\.\\pipe\\name";

int employeeCount;
employee *employees;

HANDLE *hReady;
bool *modifiedEmp;

void sortEmployees() {
    qsort(employees, employeeCount, sizeof(employee), employeeComparator);
}

void readData() {
    employees = new employee[employeeCount];
    cout << "Enter employee id, name and working hours:" << endl;
    for (int i = 1; i <= employeeCount; ++i) {
        cout << i << ")";
        cin >> employees[i - 1].id >> employees[i - 1].name >> employees[i - 1].hours;
    }
}

void writeToFile(char filename[50]) {
    fstream fin(filename, ios::binary | ios::out);
    fin.write(reinterpret_cast<char *>(employees), sizeof(employee) * employeeCount);
    fin.close();
}

employee *findEmployee(int id) {
    employee key = {};
    key.id = id;
    return (employee *) bsearch((const char *) (&key), (const char *) (employees), employeeCount, sizeof(employee),
                                employeeComparator);
}

void startProcesses(int count) {
    char buff[10];
    for (int i = 0; i < count; i++) {
        char cmdargs[80] = "..\\cmake-build-debug\\Client.exe ";
        char eventName[50] = "READY_EVENT_";
        itoa(i + 1, buff, 10);
        strcat(eventName, buff);
        strcat(cmdargs, eventName);
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        hReady[i] = CreateEvent(NULL, TRUE, FALSE, eventName);
        if (!CreateProcess(NULL, cmdargs, NULL, NULL, FALSE, CREATE_NEW_CONSOLE,
                           NULL, NULL, &si, &pi)) {
            printf("creation error.\n");
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }
}

CRITICAL_SECTION cs;

DWORD WINAPI connect(LPVOID handle) {
    HANDLE hPipe = (HANDLE) hPipe;
    employee *errorE = new employee;
    errorE->id = -1;
    while (true) {
        DWORD rBytes;
        char message[10];
        bool isRead = ReadFile(handle, message, 10, &rBytes, NULL);
        if (!isRead) {
            if (ERROR_BROKEN_PIPE == GetLastError()) {
                cout << "Client disconnected" << endl;
                break;
            } else {
                cerr << "Failed to read message" << endl;
                break;
            }
        }

        if (strlen(message) > 0) {
            char command = message[0];
            message[0] = ' ';
            int id = atoi(message);
            DWORD wBytes;
            EnterCriticalSection(&cs);
            employee *sendE = findEmployee(id);
            LeaveCriticalSection(&cs);
            if (NULL == sendE) {
                sendE = errorE;
            } else {
                int ind = sendE - employees;
                if (modifiedEmp[ind])
                    sendE = errorE;
                else {
                    switch (command) {
                        case 'w':
                            cout << "Requested to modify ID" << id;
                            modifiedEmp[ind] = true;
                            break;
                        case 'r':
                            cout << "Requested to read ID" << id;
                            break;
                        default:
                            cout << "Unknown request";
                            sendE = errorE;
                    }
                }
            }
            bool isSent = WriteFile(handle, sendE, sizeof(employee), &wBytes, NULL);
            if (isSent) cout << "Answer was sent" << endl;
            else cout << "Failed to send answer" << endl;

            if ('w' == command && sendE != errorE) {
                isRead = ReadFile(handle, sendE, sizeof(employee), &rBytes, NULL);
                if (isRead) {
                    cout << "modifiedEmp record" << endl;
                    modifiedEmp[sendE - employees] = false;
                    EnterCriticalSection(&cs);
                    sortEmployees();
                    LeaveCriticalSection(&cs);
                } else {
                    cerr << "Failed to read message" << endl;
                    break;
                }
            }
        }
    }
    FlushFileBuffers(handle);
    DisconnectNamedPipe(handle);
    CloseHandle(handle);
    delete errorE;
    return 0;
}

void openPipes(int clientCount) {
    HANDLE hPipe;
    HANDLE *hThreads = new HANDLE[clientCount];
    for (int i = 0; i < clientCount; i++) {
        hPipe = CreateNamedPipe(pipeName, PIPE_ACCESS_DUPLEX,
                                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                                PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, NULL);
        if (INVALID_HANDLE_VALUE == hPipe) {
            cerr << "Failed to create pipe" << endl;
            getch();
            return;
        }
        if (!ConnectNamedPipe(hPipe, NULL)) {
            cout << "No connected clients" << endl;
            break;
        }
        hThreads[i] = CreateThread(NULL, 0, connect, (LPVOID) hPipe, 0, NULL);
    }
    cout << "Clients connected to pipe" << endl;
    WaitForMultipleObjects(clientCount, hThreads, TRUE, INFINITE);
    cout << "Sll clients disconnected" << endl;
    delete[] hThreads;
}

int main() {
    char fileName[50];
    cout << "Enter file name and employee count\n>";
    cin >> fileName >> employeeCount;
    readData();
    writeToFile(fileName);
    sortEmployees();

    InitializeCriticalSection(&cs);
    srand(time(0));
    int clientCount = 2 + rand() % 3;
    HANDLE hstartALL = CreateEvent(NULL, TRUE, FALSE, "START_ALL");
    modifiedEmp = new bool[employeeCount];
    for (int i = 0; i < employeeCount; ++i)
        modifiedEmp[i] = false;
    hReady = new HANDLE[clientCount];
    startProcesses(clientCount);
    WaitForMultipleObjects(clientCount, hReady, TRUE, INFINITE);
    cout << "All processes are ready" << endl;
    SetEvent(hstartALL);

    openPipes(clientCount);
    for (int i = 0; i < employeeCount; i++) { employees[i].print(cout); }
    cout << "Press any key to exit ..." << endl;
    getch();
    DeleteCriticalSection(&cs);
    delete[] modifiedEmp;
    delete[] hReady;
    delete[] employees;
    return 0;
}