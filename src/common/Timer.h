#pragma once

// Timer.h — RAII scoped timer + PerformanceStats accumulator.
//
// Usage:
//   { Timer t("csv_load"); ... timed work ... }
//   // on destruction, elapsed microseconds are recorded in PerformanceStats
//
// Print all accumulated stats:
//   PerformanceStats::instance().print();

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>
#include <iostream>
#include <iomanip>
#include <climits>

// ── PerformanceStats ──────────────────────────────────────────────────────
// Singleton. Thread-safe: all mutations are protected by an internal mutex.

class PerformanceStats {
public:
    static PerformanceStats& instance() {
        static PerformanceStats inst;
        return inst;
    }

    void record(const std::string& label, long long microseconds) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto& e = entries_[label];
        e.count++;
        e.totalUs += microseconds;
        if (microseconds < e.minUs) e.minUs = microseconds;
        if (microseconds > e.maxUs) e.maxUs = microseconds;
    }

    void print() const {
        std::lock_guard<std::mutex> lock(mutex_);
        const int W = 80;
        std::cout << "\n" << std::string(W, '-') << "\n";
        std::cout << " Performance Stats\n";
        std::cout << std::string(W, '-') << "\n";
        std::cout << std::left
                  << std::setw(26) << " Operation"
                  << std::setw(7)  << "Calls"
                  << std::setw(13) << "Total µs"
                  << std::setw(11) << "Avg µs"
                  << std::setw(11) << "Min µs"
                  << std::setw(11) << "Max µs"
                  << "\n";
        std::cout << std::string(W, '-') << "\n";

        // Print in a fixed order matching section 6 of CLAUDE.md
        static const std::vector<std::string> ORDER = {
            "csv_load", "sort", "ws_transmit", "csv_save", "ws_broadcast"
        };
        for (const auto& label : ORDER) {
            auto it = entries_.find(label);
            if (it == entries_.end()) continue;
            const auto& e = it->second;
            long long avg = e.count > 0 ? e.totalUs / e.count : 0;
            long long mn  = (e.minUs == LLONG_MAX) ? 0 : e.minUs;
            std::cout << std::left
                      << " " << std::setw(25) << label
                      << std::setw(7)  << e.count
                      << std::setw(13) << e.totalUs
                      << std::setw(11) << avg
                      << std::setw(11) << mn
                      << std::setw(11) << e.maxUs
                      << "\n";
        }
        std::cout << std::string(W, '-') << "\n";
    }

private:
    PerformanceStats() = default;

    struct Entry {
        long long count   = 0;
        long long totalUs = 0;
        long long minUs   = LLONG_MAX;
        long long maxUs   = 0;
    };

    mutable std::mutex                       mutex_;
    std::unordered_map<std::string, Entry>   entries_;
};

// ── Timer ─────────────────────────────────────────────────────────────────
// RAII: records start on construction, posts elapsed µs to PerformanceStats
// on destruction.  Non-copyable.

class Timer {
public:
    explicit Timer(const std::string& label)
        : label_(label),
          start_(std::chrono::high_resolution_clock::now())
    {}

    ~Timer() {
        auto end = std::chrono::high_resolution_clock::now();
        auto us  = std::chrono::duration_cast<std::chrono::microseconds>(
                       end - start_).count();
        PerformanceStats::instance().record(label_, us);
    }

    Timer(const Timer&)            = delete;
    Timer& operator=(const Timer&) = delete;

private:
    std::string                                                   label_;
    std::chrono::time_point<std::chrono::high_resolution_clock>   start_;
};
