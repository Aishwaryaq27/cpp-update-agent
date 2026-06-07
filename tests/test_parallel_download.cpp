// test_parallel_download.cpp
// Tests that multiple std::async tasks run concurrently and all complete correctly.
// Uses a mock "download" (file copy) to avoid needing a live HTTP server.
#include <iostream>
#include <future>
#include <vector>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <cassert>
#include <cstdio>

namespace fs = std::filesystem;

// Simulates a download by sleeping then writing a file
struct MockResult {
    std::string name;
    bool        success;
    long long   duration_ms;
};

MockResult mock_download(const std::string& name, int sleep_ms) {
    auto start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));

    // Write a small output file
    std::string out = "/tmp/mock_dl_" + name + ".txt";
    std::ofstream f(out);
    f << "downloaded: " << name << "\n";

    auto end = std::chrono::steady_clock::now();
    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    return {name, true, ms};
}

int main() {
    int passed = 0, failed = 0;
    auto ok   = [&](const std::string& msg){ std::cout << "[PASS] " << msg << "\n"; ++passed; };
    auto fail = [&](const std::string& msg){ std::cout << "[FAIL] " << msg << "\n"; ++failed; };

    // ── Test 1: All tasks complete and return success ──────────────────────
    {
        std::vector<std::future<MockResult>> futures;
        std::vector<std::string> names = {"pkg-a", "pkg-b", "pkg-c"};

        for (const auto& name : names)
            futures.push_back(std::async(std::launch::async, mock_download, name, 100));

        std::vector<MockResult> results;
        for (auto& f : futures) results.push_back(f.get());

        (results.size() == 3) ? ok("All 3 futures completed") : fail("Expected 3 results");
        bool all_ok = std::all_of(results.begin(), results.end(),
                                  [](const MockResult& r){ return r.success; });
        all_ok ? ok("All downloads succeeded") : fail("Some downloads failed");

        for (const auto& name : names)
            std::remove(("/tmp/mock_dl_" + name + ".txt").c_str());
    }

    // ── Test 2: Parallel is faster than serial ─────────────────────────────
    {
        // 4 tasks x 100ms each = ~400ms serial, ~100ms parallel
        auto wall_start = std::chrono::steady_clock::now();

        std::vector<std::future<MockResult>> futures;
        for (int i = 0; i < 4; ++i)
            futures.push_back(std::async(std::launch::async, mock_download,
                                         "speed-" + std::to_string(i), 100));
        for (auto& f : futures) f.get();

        auto wall_end = std::chrono::steady_clock::now();
        long long total_ms = std::chrono::duration_cast<std::chrono::milliseconds>
                             (wall_end - wall_start).count();

        // Parallel: should finish in ~100-250ms, not 400ms
        (total_ms < 350) ?
            ok("Parallel execution faster than serial (took " + std::to_string(total_ms) + "ms)") :
            fail("Parallel took too long: " + std::to_string(total_ms) + "ms (expected < 350ms)");

        for (int i = 0; i < 4; ++i)
            std::remove(("/tmp/mock_dl_speed-" + std::to_string(i) + ".txt").c_str());
    }

    // ── Test 3: Output files actually exist after download ─────────────────
    {
        std::vector<std::future<MockResult>> futures;
        futures.push_back(std::async(std::launch::async, mock_download, "exists-check", 50));
        auto res = futures[0].get();

        std::string out = "/tmp/mock_dl_exists-check.txt";
        (fs::exists(out)) ? ok("Output file created by async task") : fail("Output file missing");
        std::remove(out.c_str());
    }

    std::cout << "\nParallel download tests: " << passed << " passed, " << failed << " failed.\n";
    return (failed == 0) ? 0 : 1;
}
