#include <windows.h>
#include <iostream>
#include <string>
#include "common.h"
#include "ringbuff.h"

using namespace std;

int main(int argc, char* argv[]) {
    //1
    if (argc < 6) {
        cout << "Usage: sender <file> <mutex> <empty> <full> <ready>\n";
        return 1;
    }
    string fileName = argv[1];
    string mutexName = argv[2];
    string emptyName = argv[3];
    string fullName = argv[4];
    string readyName = argv[5];

    // file dlya mapping
    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) { cout << "Cannot open file\n"; return 1; }

    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hMap) { cout << "Cannot create mapping\n"; CloseHandle(hFile); return 1; }
    void* base = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!base) { cout << "Cannot map view\n"; CloseHandle(hMap); CloseHandle(hFile); return 1; }

    // otryl sync
    HANDLE hMutex = OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, mutexName.c_str());
    HANDLE hEmpty = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, emptyName.c_str());
    HANDLE hFull = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, fullName.c_str());
    HANDLE hReady = OpenSemaphoreA(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, readyName.c_str());

    if (!hMutex || !hEmpty || !hFull || !hReady) { cout << "Cannot open sync objects\n"; UnmapViewOfFile(base); CloseHandle(hMap); CloseHandle(hFile); return 1; }

    //2
    ReleaseSemaphore(hReady, 1, NULL);
    cout << "Sender ready\n";

    FileHeader* hdr = reinterpret_cast<FileHeader*>(base);
    RingBuff view(base);

    //zhdu send / exit
    while (true) {
        string cmd;
        cout << "Enter command (send / exit): ";
        getline(cin, cmd);
        if (cmd == "send") {
            string msg;
            cout << "Enter message (<20 chars): ";
            getline(cin, msg);
            if (msg.size() >= MAX_MESSAGE_SIZE) msg = msg.substr(0, MAX_MESSAGE_SIZE - 1);

            //zhdu pustoy slot
            WaitForSingleObject(hEmpty, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);
            int idx = hdr->write_index;
            view.Write(idx, msg.c_str(), hdr->msg_size);
            hdr->write_index = (hdr->write_index + 1) % hdr->capacity;
            ReleaseMutex(hMutex);
            ReleaseSemaphore(hFull, 1, NULL);
            cout << "Message sent\n";
        }
        else if (cmd == "exit") {
            break;
        }
        else {
            cout << "Unknown command\n";
        }
    }

    CloseHandle(hMutex); CloseHandle(hEmpty); CloseHandle(hFull); CloseHandle(hReady);
    UnmapViewOfFile(base); CloseHandle(hMap); CloseHandle(hFile);
    return 0;
}
