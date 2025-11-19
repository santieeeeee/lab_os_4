#include "../../include/common.h"
#include "../../include/ringbuff.h"
#include <thread>
#include <chrono>

class Sender {
private:
    unique_ptr<RingBuffer> ringBuffer;
    unique_ptr<SyncManager> syncManager;
    DWORD senderId;

    HANDLE hFileMutex;
    HANDLE hMessageEvent;
    HANDLE hSpaceEvent;
    HANDLE hQueueSemaphore;
    HANDLE hReadyEvent;

public:
    Sender(const string& fileName, DWORD id)
        : senderId(id), hFileMutex(NULL), hMessageEvent(NULL),
        hSpaceEvent(NULL), hQueueSemaphore(NULL), hReadyEvent(NULL) {

        // Пункт 1: Открыть файл для передачи сообщений
        ringBuffer = make_unique<RingBuffer>(fileName, 0, 0);
        syncManager = make_unique<SyncManager>(fileName);

        hFileMutex = syncManager->OpenFileMutex();
        hMessageEvent = syncManager->OpenMessageEvent();
        hSpaceEvent = syncManager->OpenSpaceEvent();
        hQueueSemaphore = syncManager->OpenQueueSemaphore();
        hReadyEvent = syncManager->CreateReadyEvent(senderId);

        if (!hFileMutex || !hMessageEvent || !hSpaceEvent || !hQueueSemaphore || !hReadyEvent) {
            throw runtime_error("Failed to open synchronization objects");
        }
    }

    ~Sender() {
        Cleanup();
    }

    // Пункт 2: Отправить процессу Receiver сигнал на готовность к работе
    void SignalReady() {
        SetEvent(hReadyEvent);
        cout << "Sender " << senderId << " is ready!" << endl;
    }

    // Пункт 3: Выполнять циклически действия по команде с консоли
    void ProcessCommands() {
        string command;

        while (true) {
            cout << "\n=== SENDER " << senderId << " ===" << endl;
            cout << "Commands: send, status, exit" << endl;
            cout << "Enter command: ";
            getline(cin, command);

            if (command == "send") {
                SendMessage();
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
    void SendMessage() {
        // Отправить процессу Receiver сообщение
        DWORD waitResult = WaitForSingleObject(hSpaceEvent, 5000);

        if (waitResult == WAIT_OBJECT_0) {
            waitResult = WaitForSingleObject(hQueueSemaphore, 5000);

            if (waitResult == WAIT_OBJECT_0) {
                cout << "Enter message (max " << (MAX_MESSAGE_SIZE - 10) << " chars): ";
                string message;
                getline(cin, message);

                string fullMessage = "[Sender " + to_string(senderId) + "] " + message;

                WaitForSingleObject(hFileMutex, INFINITE);

                if (ringBuffer->WriteMessage(fullMessage)) {
                    cout << ">>> Message sent: " << fullMessage << endl;

                    SetEvent(hMessageEvent);

                    if (ringBuffer->IsFull()) {
                        ResetEvent(hSpaceEvent);
                    }
                }
                else {
                    cout << "Failed to write message - queue full!" << endl;
                    ReleaseSemaphore(hQueueSemaphore, 1, NULL);
                }

                ReleaseMutex(hFileMutex);
            }
            else {
                cout << "Timeout waiting for queue access" << endl;
            }
        }
        else {
            cout << "No space available in queue" << endl;
        }
    }

    void ShowStatus() {
        WaitForSingleObject(hFileMutex, INFINITE);
        DWORD messageCount = ringBuffer->GetMessageCount();
        cout << "Queue status: " << messageCount << " messages in queue" << endl;
        ReleaseMutex(hFileMutex);
    }

    void Cleanup() {
        SyncManager::SafeCloseHandle(hFileMutex);
        SyncManager::SafeCloseHandle(hMessageEvent);
        SyncManager::SafeCloseHandle(hSpaceEvent);
        SyncManager::SafeCloseHandle(hQueueSemaphore);
        SyncManager::SafeCloseHandle(hReadyEvent);
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: sender.exe <filename> <sender_id>" << endl;
        return 1;
    }

    string fileName = argv[1];
    DWORD senderId;
    try {
        senderId = stoi(argv[2]);
    }
    catch (const exception&) {
        cout << "Invalid sender ID!" << endl;
        return 1;
    }

    cout << "=== MESSAGE SENDER (ID: " << senderId << ") ===" << endl;

    try {
        Sender sender(fileName, senderId);
        sender.SignalReady();
        sender.ProcessCommands();

    }
    catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
        return 1;
    }

    cout << "Sender " << senderId << " finished." << endl;
    return 0;
}