#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <random>

using namespace std;

struct Event {
    int data;
};

class Monitor {
public:
    void addEvent(const Event& event) {
        unique_lock<mutex> lock(mutex_);
        queue_.push(event);
        cv_.notify_one(); 
    }

    Event getEvent() {
        unique_lock<mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty(); }); 
        Event event = queue_.front();
        queue_.pop();
        return event;
    }

private:
    queue<Event> queue_;
    mutex mutex_;
    condition_variable cv_;
};

void supplier(Monitor& monitor) {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist(1, 100);

    while (true) {
        Event event;
        event.data = dist(gen);

        cout << "Поставщик: Сгенерировано событие с данными " << event.data << endl;
        monitor.addEvent(event);

        cout << "Поставщик: Обработка события..." << endl;
    }
}

void consumer(Monitor& monitor) {
    while (true) {
        Event event = monitor.getEvent();

        cout << "Потребитель: Получено событие с данными " << event.data << endl;
        cout << "Потребитель: Обработка события..." << endl;
    }
}

int main() {
    Monitor monitor;
    setlocale(LC_ALL, "russian");
    thread supplierThread(supplier, ref(monitor));
    thread consumerThread(consumer, ref(monitor));

    supplierThread.join();
    consumerThread.join();

    return 0;
}