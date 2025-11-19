#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <iostream>
#include <memory>
#include <vector>
#include <stdexcept>
#include <cstring>

using namespace std;

const DWORD MAX_MESSAGE_SIZE = 20;
const DWORD DEFAULT_RECORD_COUNT = 10;

struct MessageHeader {
    DWORD totalRecords;
    DWORD recordSize;
    DWORD readIndex;
    DWORD writeIndex;
    DWORD messageCount;
};

class SyncManager {
private:
    string baseName;

public:
    SyncManager(const string& name) : baseName(name) {}

    HANDLE CreateFileMutex() {
        return CreateMutexA(NULL, FALSE, (baseName + "_FileMutex").c_str());
    }

    HANDLE OpenFileMutex() {
        return OpenMutexA(MUTEX_ALL_ACCESS, FALSE, (baseName + "_FileMutex").c_str());
    }

    HANDLE CreateMessageEvent() {
        return CreateEventA(NULL, TRUE, FALSE, (baseName + "_MessageEvent").c_str());
    }

    HANDLE OpenMessageEvent() {
        return OpenEventA(EVENT_ALL_ACCESS, FALSE, (baseName + "_MessageEvent").c_str());
    }

    HANDLE CreateSpaceEvent() {
        return CreateEventA(NULL, TRUE, TRUE, (baseName + "_SpaceEvent").c_str());
    }

    HANDLE OpenSpaceEvent() {
        return OpenEventA(EVENT_ALL_ACCESS, FALSE, (baseName + "_SpaceEvent").c_str());
    }

    HANDLE CreateReadyEvent(DWORD senderId) {
        return CreateEventA(NULL, TRUE, FALSE, (baseName + "_ReadyEvent_" + to_string(senderId)).c_str());
    }

    HANDLE OpenReadyEvent(DWORD senderId) {
        return OpenEventA(EVENT_ALL_ACCESS, FALSE, (baseName + "_ReadyEvent_" + to_string(senderId)).c_str());
    }

    HANDLE CreateQueueSemaphore(LONG initialCount, LONG maximumCount) {
        return CreateSemaphoreA(NULL, initialCount, maximumCount, (baseName + "_QueueSemaphore").c_str());
    }

    HANDLE OpenQueueSemaphore() {
        return OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, (baseName + "_QueueSemaphore").c_str());
    }

    static void SafeCloseHandle(HANDLE& handle) {
        if (handle && handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            handle = NULL;
        }
    }
};