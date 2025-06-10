#ifndef METRICS_COLLECTOR_H
#define METRICS_COLLECTOR_H

#include "metric.h"
#include <vector>
#include <memory>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <iomanip>
#include <ctime>

namespace metrics {

class Collector {
public:
    explicit Collector(const std::string& file, std::chrono::milliseconds interval = std::chrono::milliseconds(1000))
        : file_(file), interval_(interval), running_(false) {}

    ~Collector() { stop(); }

    void addMetric(std::shared_ptr<MetricBase> m) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_.push_back(std::move(m));
    }

    void start() {
        if (running_.load()) return;
        running_.store(true);
        worker_ = std::thread([this]() { this->run(); });
    }

    void stop() {
        if (!running_.load()) return;
        running_.store(false);
        if (worker_.joinable()) worker_.join();
    }

private:
    void run() {
        std::ofstream out(file_, std::ios::app);
        while (running_.load()) {
            auto next = std::chrono::steady_clock::now() + interval_;
            writeOnce(out);
            std::this_thread::sleep_until(next);
        }
        writeOnce(out);
    }

    void writeOnce(std::ofstream& out) {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto in_time_t = system_clock::to_time_t(now);
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
        std::tm tm{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm, &in_time_t);
#else
        localtime_r(&in_time_t, &tm);
#endif
        out << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        out << '.' << std::setfill('0') << std::setw(3) << ms.count();

        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& m : metrics_) {
            out << ' ' << '"' << m->name() << '"' << ' ' << m->valueAndReset();
        }
        out << '\n';
        out.flush();
    }

    std::string file_;
    std::chrono::milliseconds interval_;
    std::vector<std::shared_ptr<MetricBase>> metrics_;
    std::thread worker_;
    std::atomic<bool> running_;
    std::mutex mutex_;
};

}

#endif
