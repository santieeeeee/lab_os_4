#include "../include/common.h"
#include "../include/ringbuff.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

using namespace std;

class RingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Очищаем возможные предыдущие тестовые файлы
        DeleteFileA("test_ringbuffer.bin");
    }

    void TearDown() override {
        // Убираем за собой после тестов
        DeleteFileA("test_ringbuffer.bin");
    }
};

//Тест 1: Создание RingBuffer
TEST_F(RingBufferTest, Creation) {
    RingBuffer buffer("test_ringbuffer.bin", 5, 20);

    EXPECT_EQ(buffer.GetMessageCount(), 0);
    EXPECT_TRUE(buffer.IsEmpty());
    EXPECT_FALSE(buffer.IsFull());
}

//Тест 2: Запись и чтение сообщений
TEST_F(RingBufferTest, WriteAndRead) {
    RingBuffer buffer("test_ringbuffer.bin", 3, 20);

    // Тест записи
    EXPECT_TRUE(buffer.WriteMessage("Hello World"));
    EXPECT_EQ(buffer.GetMessageCount(), 1);
    EXPECT_FALSE(buffer.IsEmpty());

    // Тест чтения
    string message;
    EXPECT_TRUE(buffer.ReadMessage(message));
    EXPECT_EQ(message, "Hello World");
    EXPECT_EQ(buffer.GetMessageCount(), 0);
    EXPECT_TRUE(buffer.IsEmpty());
}

//Тест 3: Граничные условия - переполнение
TEST_F(RingBufferTest, Overflow) {
    RingBuffer buffer("test_ringbuffer.bin", 2, 20);

    // Заполняем буфер
    EXPECT_TRUE(buffer.WriteMessage("Message 1"));
    EXPECT_TRUE(buffer.WriteMessage("Message 2"));
    EXPECT_TRUE(buffer.IsFull());

    // Попытка записи в полный буфер
    EXPECT_FALSE(buffer.WriteMessage("Message 3"));

    // Освобождаем место и снова пишем
    string message;
    EXPECT_TRUE(buffer.ReadMessage(message));
    EXPECT_FALSE(buffer.IsFull());
    EXPECT_TRUE(buffer.WriteMessage("Message 3"));
}

//Тест 4: Граничные условия - чтение из пустого буфера
TEST_F(RingBufferTest, ReadFromEmpty) {
    RingBuffer buffer("test_ringbuffer.bin", 2, 20);

    string message;
    EXPECT_FALSE(buffer.ReadMessage(message));
    EXPECT_TRUE(buffer.IsEmpty());
}

//Тест 5: Кольцевое поведение (wrap-around)
TEST_F(RingBufferTest, WrapAround) {
    RingBuffer buffer("test_ringbuffer.bin", 3, 20);

    // Записываем и читаем, чтобы вызвать переполнение индексов
    buffer.WriteMessage("Message 1");
    buffer.WriteMessage("Message 2");

    string message;
    buffer.ReadMessage(message); // Освобождаем первое место

    buffer.WriteMessage("Message 3"); // Должно записаться в освободившееся место
    buffer.WriteMessage("Message 4"); // Должно перезаписать начало

    EXPECT_EQ(buffer.GetMessageCount(), 3);
    EXPECT_TRUE(buffer.IsFull());
}

// Фикстура для тестов синхронизации
class SyncObjectsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Генерируем уникальное имя для каждого теста
        testId = to_string(GetCurrentProcessId()) + "_" + to_string(GetTickCount());
        syncManager = make_unique<SyncManager>("test_sync_" + testId);
    }

    void TearDown() override {
        // Закрываем все handles
        if (hMutex) CloseHandle(hMutex);
        if (hEvent) CloseHandle(hEvent);
        if (hSemaphore) CloseHandle(hSemaphore);
    }

    string testId;
    unique_ptr<SyncManager> syncManager;
    HANDLE hMutex = NULL;
    HANDLE hEvent = NULL;
    HANDLE hSemaphore = NULL;
};

//Тест 6: Создание и работа с мьютексом
TEST_F(SyncObjectsTest, MutexOperations) {
    hMutex = syncManager->CreateFileMutex();
    EXPECT_NE(hMutex, nullptr);

    // Пытаемся захватить мьютекс
    DWORD result = WaitForSingleObject(hMutex, 1000);
    EXPECT_EQ(result, WAIT_OBJECT_0);

    // Освобождаем мьютекс
    BOOL releaseResult = ReleaseMutex(hMutex);
    EXPECT_EQ(releaseResult, TRUE);
}

//Тест 7: Создание и работа с событиями
TEST_F(SyncObjectsTest, EventOperations) {
    hEvent = syncManager->CreateMessageEvent();
    EXPECT_NE(hEvent, nullptr);

    // Проверяем начальное состояние (должно быть несигнальное)
    DWORD result = WaitForSingleObject(hEvent, 100);
    EXPECT_EQ(result, WAIT_TIMEOUT);

    // Устанавливаем событие
    EXPECT_TRUE(SetEvent(hEvent));

    // Теперь должно быть сигнальное
    result = WaitForSingleObject(hEvent, 1000);
    EXPECT_EQ(result, WAIT_OBJECT_0);

    // Сбрасываем событие
    EXPECT_TRUE(ResetEvent(hEvent));
}

//Тест 8: Создание и работа с семафором
TEST_F(SyncObjectsTest, SemaphoreOperations) {
    hSemaphore = syncManager->CreateQueueSemaphore(3, 5);
    EXPECT_NE(hSemaphore, nullptr);

    // Захватываем семафор 3 раза (начальное значение)
    for (int i = 0; i < 3; i++) {
        DWORD result = WaitForSingleObject(hSemaphore, 1000);
        EXPECT_EQ(result, WAIT_OBJECT_0);
    }

    // Попытка захватить семафор 4-й раз (должен быть таймаут)
    DWORD timeoutResult = WaitForSingleObject(hSemaphore, 100);
    EXPECT_EQ(timeoutResult, WAIT_TIMEOUT);

    // Освобождаем семафор
    EXPECT_TRUE(ReleaseSemaphore(hSemaphore, 1, NULL));
}

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Генерируем уникальное имя для каждого теста
        testId = to_string(GetCurrentProcessId()) + "_" + to_string(GetTickCount());
        fileName = "test_integration_" + testId + ".bin";
        syncBaseName = "test_integration_" + testId;
    }

    void TearDown() override {
        // Закрываем все handles
        if (hMutex) CloseHandle(hMutex);
        if (hMessageEvent) CloseHandle(hMessageEvent);
        if (hSpaceEvent) CloseHandle(hSpaceEvent);
        if (hSemaphore) CloseHandle(hSemaphore);
        DeleteFileA(fileName.c_str());
    }

    string testId;
    string fileName;
    string syncBaseName;
    HANDLE hMutex = NULL;
    HANDLE hMessageEvent = NULL;
    HANDLE hSpaceEvent = NULL;
    HANDLE hSemaphore = NULL;
};

//Тест 9: Упрощенный интеграционный тест - только буфер
TEST_F(IntegrationTest, SimpleBufferIntegration) {
    RingBuffer buffer(fileName, 3, 20);

    // Простая проверка записи и чтения без синхронизации
    EXPECT_TRUE(buffer.WriteMessage("Simple Test"));
    EXPECT_EQ(buffer.GetMessageCount(), 1);

    string message;
    EXPECT_TRUE(buffer.ReadMessage(message));
    EXPECT_EQ(message, "Simple Test");
    EXPECT_EQ(buffer.GetMessageCount(), 0);
}

//Тест 10: Многопоточное взаимодействие
TEST_F(IntegrationTest, ConcurrentAccess) {
    RingBuffer buffer(fileName, 2, 20);
    SyncManager sync(syncBaseName);

    hMutex = sync.CreateFileMutex();
    hMessageEvent = sync.CreateMessageEvent();
    hSpaceEvent = sync.CreateSpaceEvent();
    hSemaphore = sync.CreateQueueSemaphore(2, 2);

    atomic<int> messagesSent{ 0 };
    atomic<int> messagesReceived{ 0 };
    const int TOTAL_MESSAGES = 5;

    // Поток-отправитель
    auto senderThread = [&]() {
        for (int i = 0; i < TOTAL_MESSAGES; i++) {
            DWORD spaceResult = WaitForSingleObject(hSpaceEvent, INFINITE);
            if (spaceResult != WAIT_OBJECT_0) continue;

            DWORD semaphoreResult = WaitForSingleObject(hSemaphore, INFINITE);
            if (semaphoreResult != WAIT_OBJECT_0) continue;

            DWORD mutexResult = WaitForSingleObject(hMutex, INFINITE);
            if (mutexResult != WAIT_OBJECT_0) {
                ReleaseSemaphore(hSemaphore, 1, NULL);
                continue;
            }

            if (buffer.WriteMessage("Concurrent " + to_string(i))) {
                messagesSent++;
            }

            SetEvent(hMessageEvent);
            ReleaseMutex(hMutex);
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        };

    // Поток-получатель
    auto receiverThread = [&]() {
        for (int i = 0; i < TOTAL_MESSAGES; i++) {
            DWORD messageResult = WaitForSingleObject(hMessageEvent, INFINITE);
            if (messageResult != WAIT_OBJECT_0) continue;

            DWORD mutexResult = WaitForSingleObject(hMutex, INFINITE);
            if (mutexResult != WAIT_OBJECT_0) continue;

            string message;
            if (buffer.ReadMessage(message)) {
                messagesReceived++;
            }

            SetEvent(hSpaceEvent);
            ReleaseSemaphore(hSemaphore, 1, NULL);
            ReleaseMutex(hMutex);
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        };

    thread sender(senderThread);
    thread receiver(receiverThread);

    sender.join();
    receiver.join();

    EXPECT_GT(messagesSent, 0);
    EXPECT_GT(messagesReceived, 0);
    EXPECT_EQ(messagesSent, messagesReceived);
}

//Тест 11: Производительность - массовая отправка сообщений
TEST(PerformanceTest, BulkMessageTransfer) {
    const int MESSAGE_COUNT = 50; // Уменьшим для стабильности
    string fileName = "test_performance.bin";
    string syncBaseName = "test_performance";

    RingBuffer buffer(fileName, MESSAGE_COUNT, 20);
    SyncManager sync(syncBaseName);

    HANDLE hMutex = sync.CreateFileMutex();
    HANDLE hMessageEvent = sync.CreateMessageEvent();
    HANDLE hSpaceEvent = sync.CreateSpaceEvent();
    HANDLE hSemaphore = sync.CreateQueueSemaphore(MESSAGE_COUNT, MESSAGE_COUNT);

    auto startTime = chrono::high_resolution_clock::now();

    // Отправка сообщений
    for (int i = 0; i < MESSAGE_COUNT; i++) {
        WaitForSingleObject(hSpaceEvent, INFINITE);
        WaitForSingleObject(hSemaphore, INFINITE);
        WaitForSingleObject(hMutex, INFINITE);

        buffer.WriteMessage("Message " + to_string(i));
        SetEvent(hMessageEvent);
        ReleaseMutex(hMutex);
    }

    // Получение сообщений
    for (int i = 0; i < MESSAGE_COUNT; i++) {
        WaitForSingleObject(hMessageEvent, INFINITE);
        WaitForSingleObject(hMutex, INFINITE);

        string message;
        buffer.ReadMessage(message);
        SetEvent(hSpaceEvent);
        ReleaseSemaphore(hSemaphore, 1, NULL);
        ReleaseMutex(hMutex);
    }

    auto endTime = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(endTime - startTime);

    // Проверяем что производительность адекватна
    EXPECT_LT(duration.count(), 10000); // Должно уложиться в 10 секунд

    cout << "Performance test: " << MESSAGE_COUNT << " messages in "
        << duration.count() << " ms" << endl;

    // Очистка
    CloseHandle(hMutex);
    CloseHandle(hMessageEvent);
    CloseHandle(hSpaceEvent);
    CloseHandle(hSemaphore);
    DeleteFileA(fileName.c_str());
}

//Тест 12: Обработка ошибок - неверные параметры
TEST(ErrorHandlingTest, InvalidParameters) {
    // Попытка создать буфер с нулевым размером
    EXPECT_THROW({
        RingBuffer buffer("test_invalid.bin", 0, 20);
        }, runtime_error);
}

// Тест 13: Восстановление после ошибок
TEST(ErrorHandlingTest, RecoveryAfterErrors) {
    string fileName = "test_recovery.bin";
    RingBuffer buffer(fileName, 2, 20);

    // Заполняем буфер
    buffer.WriteMessage("Message 1");
    buffer.WriteMessage("Message 2");

    // Попытка записи в полный буфер (должна вернуть false)
    EXPECT_FALSE(buffer.WriteMessage("Message 3"));

    // Освобождаем место
    string message;
    buffer.ReadMessage(message);

    // Теперь запись должна работать
    EXPECT_TRUE(buffer.WriteMessage("Message 3"));

    DeleteFileA(fileName.c_str());
}

//Тест 14: Проверка начального состояния событий
TEST_F(SyncObjectsTest, EventInitialStates) {
    // Проверяем начальное состояние MessageEvent (должно быть несигнальное)
    hEvent = syncManager->CreateMessageEvent();
    DWORD result = WaitForSingleObject(hEvent, 100);
    EXPECT_EQ(result, WAIT_TIMEOUT) << "MessageEvent should be initially non-signaled";

    // Проверяем начальное состояние SpaceEvent (должно быть сигнальное)
    HANDLE hSpaceEvent = syncManager->CreateSpaceEvent();
    result = WaitForSingleObject(hSpaceEvent, 100);
    EXPECT_EQ(result, WAIT_OBJECT_0) << "SpaceEvent should be initially signaled";

    CloseHandle(hSpaceEvent);
}

// Главная функция для запуска тестов
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Настраиваем вывод тестов
    ::testing::GTEST_FLAG(color) = "yes";
    ::testing::GTEST_FLAG(print_time) = true;

    return RUN_ALL_TESTS();
}