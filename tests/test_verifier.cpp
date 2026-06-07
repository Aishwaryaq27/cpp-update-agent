// test_verifier.cpp
// Tests: correct hash accepted, wrong hash rejected, missing file handled
#include "verifier.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <cstdio>

int main() {
    int passed = 0, failed = 0;

    auto ok   = [&](const std::string& msg){ std::cout << "[PASS] " << msg << "\n"; ++passed; };
    auto fail = [&](const std::string& msg){ std::cout << "[FAIL] " << msg << "\n"; ++failed; };

    const std::string test_file = "/tmp/test_verifier_input.txt";
    {
        std::ofstream f(test_file);
        f << "hello world\n";
    }

    // Compute expected hash dynamically so it works on any platform
    // Use openssl CLI to get ground-truth hash
    FILE* pipe = popen(("openssl dgst -sha256 " + test_file).c_str(), "r");
    char buf[256] = {};
    if (pipe) { fgets(buf, sizeof(buf), pipe); pclose(pipe); }
    std::string openssl_out(buf);
    // output format: "SHA2-256(path)= hexhash\n" or "SHA256(path)= hexhash\n"
    std::string expected_hash;
    auto eq = openssl_out.rfind('=');
    if (eq != std::string::npos) {
        expected_hash = openssl_out.substr(eq + 2);
        while (!expected_hash.empty() &&
               (expected_hash.back() == '\n' || expected_hash.back() == '\r' || expected_hash.back() == ' '))
            expected_hash.pop_back();
    }

    Verifier v;

    // Test 1: correct hash accepted
    if (!expected_hash.empty() && v.verify_sha256(test_file, expected_hash))
        ok("Correct hash accepted");
    else
        fail("Correct hash rejected (hash: '" + expected_hash + "')");

    // Test 2: wrong hash rejected
    if (!v.verify_sha256(test_file, std::string(64, '0')))
        ok("Wrong hash correctly rejected");
    else
        fail("Wrong hash was not rejected");

    // Test 3: missing file returns false gracefully
    if (!v.verify_sha256("/tmp/no_such_file_xyz.txt", std::string(64, '0')))
        ok("Missing file handled gracefully");
    else
        fail("Missing file should return false");

    std::remove(test_file.c_str());

    std::cout << "\nVerifier tests: " << passed << " passed, " << failed << " failed.\n";
    return (failed == 0) ? 0 : 1;
}
