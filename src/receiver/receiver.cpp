#include "../../include/common.h"
#include "../../include/ringbuff.h"
#include <vector>
#include <thread>
#include <chrono>

class Receiver {
private:
    unique_ptr<RingBuffer> ringBuffer;
    unique_ptr<SyncManager> syncManager;
    vector<HANDLE> senderProcesses;
    vector<HANDLE> readyEvents;

    HANDLE hFileMutex;
    HANDLE hMessageEvent;
    HANDLE hSpaceEvent;
    HANDLE hQueueSemaphore;
    DWORD totalRecords;

public:
    Receiver(const string& fileName, DWORD recordCount)
        : hFileMutex(NULL), hMessageEvent(NULL), hSpaceEvent(NULL),
        hQueueSemaphore(NULL), totalRecords(recordCount) {

        // Пункт 1: Создать бинарный файл для сообщений
        ringBuffer = make_unique<RingBuffer>(fileName, recordCount);
        syncManager = make_unique<SyncManager>(fileName);

        hFileMutex = syncManager->CreateFileMutex();
        hMessageEvent = syncManager->CreateMessageEvent();
        hSpaceEvent = syncManager->CreateSpaceEvent();
        hQueueSemaphore = syncManager->CreateQueueSemaphore(static_cast<LONG>(recordCount), static_cast<LONG>(recordCount));

        if (!hFileMutex || !hMessageEvent || !hSpaceEvent || !hQueueSemaphore) {
            throw runtime_error("Failed to create synchronization objects");
        }
    }

    ~Receiver() {
        Cleanup();
    }

    // Пункт 3: Запустить заданное количество процессов Sender
    bool StartSenders(const string& fileName, DWORD senderCount) {
        for (DWORD i = 0; i < senderCount; i++) {
            STARTUPINFOA si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            string commandLine = "sender.exe " + fileName + " " + to_string(i);

            if (!CreateProcessA(NULL,
                const_cast<LPSTR>(commandLine.c_str()),
                NULL, NULL, FALSE,
                CREATE_NEW_CONSOLE,
                NULL, NULL, &si, &pi)) {
                cout << "Error starting Sender process " << i << endl;
                return false;
            }

            CloseHandle(pi.hThread);
            CloseHandle(pi.hProcess);

            HANDLE hReadyEvent = syncManager->CreateReadyEvent(i);
            readyEvents.push_back(hReadyEvent);

            Sleep(100);
        }

        return true;
    }

    // Пункт 4: Ждать сигнал на готовность от всех процессов Sender
    bool WaitForSendersReady() {
        if (readyEvents.empty()) {
            cout << "No senders to wait for!" << endl;
            return false;
        }

        cout << "Waiting for " << readyEvents.size() << " sender(s) to be ready..." << endl;

        DWORD result = WaitForMultipleObjects(
            static_cast<DWORD>(readyEvents.size()),
            readyEvents.data(),
            TRUE,
            INFINITE
        );

        if (result >= WAIT_OBJECT_0 && result < WAIT_OBJECT_0 + readyEvents.size()) {
            cout << "All senders are ready!" << endl;
            return true;
        }
        else {
            cout << "Error waiting for senders: " << result << endl;
            return false;
        }
    }

    // Пункт 5: Выполнять циклически действия по команде с консоли
    void ProcessCommands() {
        this_thread::sleep_for(chrono::milliseconds(500));

        string command;

        while (true) {
            cout << "\n=== RECEIVER ===" << endl;
            cout << "Commands: read, status, exit" << endl;
            cout << "Enter command: ";
            getline(cin, command);

            if (command == "read") {
                ReadMessage();
            }
            else if (command == "status") {
                ShowStatus();
            }
            else if (command == "exit") {
                break;
            }
            else {
                cout << "Unknown command!" << endl;
            }
        }
    }

private:
    void ReadMessage() {
        // Читать сообщение из бинарного файла
        DWORD waitResult = WaitForSingleObject(hMessageEvent, 5000);

        if (waitResult == WAIT_OBJECT_0) {
            WaitForSingleObject(hFileMutex, INFINITE);

            string message;
            if (ringBuffer->ReadMessage(message)) {
                cout << ">>> Received: " << message << endl;

                if (ringBuffer->IsEmpty()) {
                    ResetEvent(hMessageEvent);
                }
                SetEvent(hSpaceEvent);

                ReleaseSemaphore(hQueueSemaphore, 1, NULL);
            }
            else {
                cout << "No message available!" << endl;
                ResetEvent(hMessageEvent);
            }

            ReleaseMutex(hFileMutex);
        }
        else if (waitResult == WAIT_TIMEOUT) {
            cout << "No messages received within timeout" << endl;
        }
        else {
            cout << "Error waiting for message: " << waitResult << endl;
        }
    }

    void ShowStatus() {
        WaitForSingleObject(hFileMutex, INFINITE);
        DWORD messageCount = ringBuffer->GetMessageCount();
        DWORD freeSlots = totalRecords - messageCount;
        cout << "Queue status: " << messageCount
            << " messages, " << freeSlots
            << " free slots" << endl;
        ReleaseMutex(hFileMutex);
    }

    void Cleanup() {
        for (auto hProcess : senderProcesses) {
            if (hProcess) {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }

        for (auto hEvent : readyEvents) {
            SyncManager::SafeCloseHandle(hEvent);
        }

        SyncManager::SafeCloseHandle(hFileMutex);
        SyncManager::SafeCloseHandle(hMessageEvent);
        SyncManager::SafeCloseHandle(hSpaceEvent);
        SyncManager::SafeCloseHandle(hQueueSemaphore);
    }
};

int main() {
    string fileName;
    DWORD recordCount, senderCount;

    cout << "=== MESSAGE RECEIVER ===" << endl;

    // Пункт 1: Ввести с консоли имя файла и количество записей
    cout << "Enter binary file name: ";
    getline(cin, fileName);

    cout << "Enter number of records in queue: ";
    cin >> recordCount;

    // Пункт 2: Ввести с консоли количество процессов Sender
    cout << "Enter number of Sender processes: ";
    cin >> senderCount;
    cin.ignore();

    try {
        Receiver receiver(fileName, recordCount);

        if (!receiver.StartSenders(fileName, senderCount)) {
            cout << "Failed to start sender processes!" << endl;
            return 1;
        }

        cout << "Waiting for sender windows to open..." << endl;
        Sleep(2000);

        if (!receiver.WaitForSendersReady()) {
            cout << "Senders not ready!" << endl;
            return 1;
        }

        receiver.ProcessCommands();

    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
        return 1;
    }

    cout << "Receiver finished." << endl;
    return 0;
}