#ifndef METRIC_H
#define METRIC_H

#include <atomic>
#include <string>
#include <sstream>

namespace metrics {

class MetricBase {
public:
    explicit MetricBase(std::string name) : name_(std::move(name)) {}
    virtual ~MetricBase() = default;

    const std::string& name() const { return name_; }

    virtual std::string valueAndReset() = 0;

private:
    std::string name_;
};

template <typename T>
class Gauge : public MetricBase {
public:
    explicit Gauge(const std::string& name) : MetricBase(name), value_(T{}) {}

    void record(T v) { value_.store(v, std::memory_order_relaxed); }

    std::string valueAndReset() override {
        T v = value_.exchange(T{}, std::memory_order_relaxed);
        std::ostringstream ss;
        ss << v;
        return ss.str();
    }

private:
    std::atomic<T> value_;
};

template <typename T>
class Counter : public MetricBase {
public:
    explicit Counter(const std::string& name) : MetricBase(name), value_(T{}) {}

    void increment(T v = 1) { value_.fetch_add(v, std::memory_order_relaxed); }

    std::string valueAndReset() override {
        T v = value_.exchange(T{}, std::memory_order_relaxed);
        std::ostringstream ss;
        ss << v;
        return ss.str();
    }

private:
    std::atomic<T> value_;
};

}

#endif
