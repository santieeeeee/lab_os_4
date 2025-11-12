#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include "common.h"
#include "ringbuff.h"

using namespace std;

int main() {
    //1
    string fileName;
    cout << "Enter file name: ";
    getline(cin, fileName);

    int N = 0;
    cout << "Enter capacity (number of records): ";
    cin >> N;
    cin.ignore();

    // sync names
    string mutexName = MakeName("MsgMutex", fileName);
    string emptyName = MakeName("MsgEmpty", fileName);
    string fullName = MakeName("MsgFull", fileName);
    string readyName = MakeName("MsgReady", fileName);

    HANDLE hFile = CreateFileA(fileName.c_str(), GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Cannot create file\n";
        return 1;
    }

    SIZE_T msg_size = MAX_MESSAGE_SIZE;
    SIZE_T totalSize = sizeof(FileHeader) + (SIZE_T)N * msg_size;
    LARGE_INTEGER li; li.QuadPart = (LONGLONG)totalSize;
    SetFilePointerEx(hFile, li, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);

    HANDLE hMap = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (!hMap) { cout << "Mapping failed\n"; CloseHandle(hFile); return 1; }
    void* base = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!base) { cout << "MapView failed\n"; CloseHandle(hMap); CloseHandle(hFile); return 1; }

    //
    HANDLE hMutex = CreateMutexA(NULL, FALSE, mutexName.c_str());
    HANDLE hEmpty = CreateSemaphoreA(NULL, N, N, emptyName.c_str());
    HANDLE hFull = CreateSemaphoreA(NULL, 0, N, fullName.c_str());
    HANDLE hReady = CreateSemaphoreA(NULL, 0, 1024, readyName.c_str());

    //2
    int M = 0;
    cout << "Enter number of Sender processes: ";
    cin >> M;
    cin.ignore();

    //3
    vector<HANDLE> childs;
    for (int i = 0; i < M; ++i) {
        CHAR exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        string exeFull(exePath);
        size_t pos = exeFull.find_last_of("\\/");
        string exeDir = (pos == string::npos) ? "." : exeFull.substr(0, pos);
        string senderPath = exeDir + "\\sender.exe";

        string cmdLine = "\"" + senderPath + "\" \"" + fileName + "\" \"" + mutexName + "\" \"" + emptyName + "\" \"" + fullName + "\" \"" + readyName + "\"";
        vector<char> cmd(cmdLine.begin(), cmdLine.end()); cmd.push_back('\0');

        STARTUPINFOA si; PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        if (CreateProcessA(NULL, cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            childs.push_back(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else {
            cout << "Failed to create sender (maybe file not found): " << senderPath << "\n";
        }
    }

    //4
    for (int i = 0; i < M; ++i) {
        WaitForSingleObject(hReady, INFINITE);
    }
    cout << "All senders ready\n";

    FileHeader* hdr = reinterpret_cast<FileHeader*>(base);
    hdr->capacity = N;
    hdr->msg_size = (int)msg_size;
    hdr->read_index = 0;
    hdr->write_index = 0;

    RingBuff view(base);

    //5
    while (true) {
        string cmd;
        cout << "Enter command (read / exit): ";
        getline(cin, cmd);
        if (cmd == "read") {
            WaitForSingleObject(hFull, INFINITE);
            WaitForSingleObject(hMutex, INFINITE);
            int idx = hdr->read_index;
            string msg = view.ReadAt(idx, hdr->msg_size);
            hdr->read_index = (hdr->read_index + 1) % hdr->capacity;
            ReleaseMutex(hMutex);
            ReleaseSemaphore(hEmpty, 1, NULL);
            cout << "Received: " << msg << endl;
        }
        else if (cmd == "exit") {
            break;
        }
        else {
            cout << "Unknown command\n";
        }
    }

    for (auto h : childs) CloseHandle(h);
    CloseHandle(hEmpty); CloseHandle(hFull); CloseHandle(hReady); CloseHandle(hMutex);
    UnmapViewOfFile(base); CloseHandle(hMap); CloseHandle(hFile);
    return 0;
}
