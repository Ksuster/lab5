#include <iostream>
#include <conio.h>
#include <windows.h>
#include "employee.h"

using namespace std;

const string pipeName = "\\\\.\\pipe\\name";

void connect(HANDLE hPipe) {

    while (true) {
        char command[10];
        cout << "Input r to read or w to write command and employee id: " << endl;
        cin.getline(command, 10, '\n');
        if (cin.eof()) {
            return;
        }
        DWORD wBytes;

        if (!WriteFile(hPipe, command, 10, &wBytes, NULL)) {
            cerr << "Message was not sent" << endl;
            getch();
            return;
        }

        employee emp;
        DWORD rBytes;

        if (ReadFile(hPipe, &emp, sizeof(emp), &rBytes, NULL))
            cerr << "Answer was not received" << endl;

        else {
            if (emp.id < 0) {
                cerr << "Employee not found" << endl;
                continue;
            } else {
                emp.print(cout);
                if ('w' == command[0]) {
                    cout << "Enter employee id, name and working hours:\n>" << flush;
                    cin >> emp.id >> emp.name >> emp.hours;
                    cin.ignore(2, '\n');
                    if (WriteFile(hPipe, &emp, sizeof(emp), &wBytes, NULL))
                        cout << "Record has been sent" << endl;

                    else {
                        cerr << "Failed to send" << endl;
                        getch();
                        break;
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv) {
    HANDLE hReadyEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, argv[1]);
    HANDLE hStartEvent = OpenEvent(SYNCHRONIZE, FALSE, "START_ALL");

    if (NULL == hStartEvent || NULL == hReadyEvent) {
        cerr << "Event error" << endl;
        getch();
        return GetLastError();
    }
    SetEvent(hReadyEvent);
    WaitForSingleObject(hStartEvent, INFINITE);
    cout << "Process started" << endl;

    HANDLE hPipe;
    while (true) {
        hPipe = CreateFile(pipeName.c_str(), GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ, NULL,
                           OPEN_EXISTING, 0, NULL);
        if (INVALID_HANDLE_VALUE != hPipe) {
            break;
        }
        if (!WaitNamedPipe(pipeName.c_str(), 2000)) {
            cout << "Out of time" << endl;
            getch();
            return 0;
        }
    }
    cout << "Connected to pipe" << endl;
    connect(hPipe);
    return 0;
}