#pragma once
#include "common.h"

class RingBuffer {
private:
    HANDLE hFile;
    HANDLE hFileMapping;
    MessageHeader* pHeader;
    char* pData;
    string fileName;
    DWORD totalSize;

public:
    RingBuffer(const string& name, DWORD recordCount, DWORD recordSize = MAX_MESSAGE_SIZE);
    ~RingBuffer();

    bool WriteMessage(const string& message);
    bool ReadMessage(string& message);
    bool IsEmpty() const;
    bool IsFull() const;
    DWORD GetMessageCount() const;
};

inline RingBuffer::RingBuffer(const string& name, DWORD recordCount, DWORD recordSize)
    : fileName(name), hFile(INVALID_HANDLE_VALUE), hFileMapping(NULL),
    pHeader(nullptr), pData(nullptr) {

    totalSize = sizeof(MessageHeader) + recordCount * recordSize;

    if (recordCount > 0) {
        hFile = CreateFileA(name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }
    else {
        hFile = CreateFileA(name.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    }

    if (hFile == INVALID_HANDLE_VALUE) {
        throw runtime_error("Cannot open file: " + name);
    }

    if (recordCount > 0) {
        SetFilePointer(hFile, totalSize, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);
    }

    hFileMapping = CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, totalSize, NULL);
    if (!hFileMapping) {
        CloseHandle(hFile);
        throw runtime_error("Cannot create file mapping");
    }

    pHeader = static_cast<MessageHeader*>(MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, totalSize));
    if (!pHeader) {
        CloseHandle(hFileMapping);
        CloseHandle(hFile);
        throw runtime_error("Cannot map view of file");
    }

    if (recordCount > 0) {
        pHeader->totalRecords = recordCount;
        pHeader->recordSize = recordSize;
        pHeader->readIndex = 0;
        pHeader->writeIndex = 0;
        pHeader->messageCount = 0;
    }

    pData = reinterpret_cast<char*>(pHeader + 1);
}

inline RingBuffer::~RingBuffer() {
    if (pHeader) UnmapViewOfFile(pHeader);
    if (hFileMapping) CloseHandle(hFileMapping);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
}

inline bool RingBuffer::WriteMessage(const string& message) {
    if (IsFull()) return false;

    string truncated = message.substr(0, pHeader->recordSize - 1);
    char* record = pData + (pHeader->writeIndex * pHeader->recordSize);

    strcpy_s(record, pHeader->recordSize, truncated.c_str());

    pHeader->writeIndex = (pHeader->writeIndex + 1) % pHeader->totalRecords;
    pHeader->messageCount++;

    return true;
}

inline bool RingBuffer::ReadMessage(string& message) {
    if (IsEmpty()) return false;

    char* record = pData + (pHeader->readIndex * pHeader->recordSize);
    message = record;

    memset(record, 0, pHeader->recordSize);
    pHeader->readIndex = (pHeader->readIndex + 1) % pHeader->totalRecords;
    pHeader->messageCount--;

    return true;
}

inline bool RingBuffer::IsEmpty() const {
    return pHeader->messageCount == 0;
}

inline bool RingBuffer::IsFull() const {
    return pHeader->messageCount >= pHeader->totalRecords;
}

inline DWORD RingBuffer::GetMessageCount() const {
    return pHeader->messageCount;
}