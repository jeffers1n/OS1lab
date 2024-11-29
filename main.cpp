#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>

using namespace std;

struct Task {
    int value;
};

class TaskMonitor {
public:
    void pushTask(const Task& task) {
        unique_lock<mutex> lock(queueMutex_);
        taskQueue_.push(task);
        conditionVar_.notify_one();
    }

    Task popTask() {
        unique_lock<mutex> lock(queueMutex_);
        conditionVar_.wait(lock, [this] { return !taskQueue_.empty(); });
        Task task = taskQueue_.front();
        taskQueue_.pop();
        return task;
    }

private:
    queue<Task> taskQueue_;
    mutex queueMutex_;
    condition_variable conditionVar_;
};

void producer(TaskMonitor& monitor) {
    random_device randomDevice;
    mt19937 generator(randomDevice());
    uniform_int_distribution<> distribution(1, 100);

    while (true) {
        Task task;
        task.value = distribution(generator);

        cout << "Производитель: Создана задача с данными " << task.value << endl;
        monitor.pushTask(task);

        cout << "Производитель: Переработка задачи..." << endl;

        // Задержка для улучшения восприятия вывода
        this_thread::sleep_for(chrono::milliseconds(1000));
    }
}

void consumer(TaskMonitor& monitor) {
    while (true) {
        Task task = monitor.popTask();
        cout << "Потребитель: Получена задача с данными " << task.value << endl;
        cout << "Потребитель: Переработка задачи..." << endl;

        // Задержка для улучшения восприятия вывода
        this_thread::sleep_for(chrono::milliseconds(10000));
    }
}

int main() {
    TaskMonitor taskMonitor;
    setlocale(LC_ALL, "russian");
    thread prodThread(producer, ref(taskMonitor));
    thread consThread(consumer, ref(taskMonitor));

    prodThread.join();
    consThread.join();

    return 0;
}
