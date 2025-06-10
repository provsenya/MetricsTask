#include "metrics/collector.h"
#include <thread>
#include <random>
#include <iostream>

int main() {
    using namespace metrics;
    auto cpu = std::make_shared<Gauge<double>>("CPU");
    auto http = std::make_shared<Counter<int>>("HTTP requests RPS");

    Collector collector("metrics.log", std::chrono::milliseconds(1000));
    collector.addMetric(cpu);
    collector.addMetric(http);
    collector.start();

    std::atomic<bool> running{true};

    std::thread cpu_thread([&]() {
        std::mt19937 gen{std::random_device{}()};
        std::uniform_real_distribution<double> dist(0.0, 2.0);
        while (running.load()) {
            cpu->record(dist(gen));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::thread http_thread([&]() {
        std::mt19937 gen{std::random_device{}()};
        std::uniform_int_distribution<int> dist(0, 5);
        while (running.load()) {
            http->increment(dist(gen));
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(5));
    running.store(false);

    cpu_thread.join();
    http_thread.join();

    collector.stop();
    std::cout << "Metrics written to metrics.log" << std::endl;
    return 0;
}

