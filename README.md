# cpp-update-agent

A reliable, small-scale **software update/patch agent** written in modern C++17 for Linux.  
Built as a portfolio project demonstrating systems programming, networking, security, and concurrency.

---

## Features

| Feature | Details |
|---|---|
| **Manifest-driven updates** | Fetches a JSON manifest from a remote URL listing packages with versions and download URLs |
| **Parallel downloads** | Downloads and verifies all pending packages concurrently using `std::async` / `std::future` |
| **Secure downloads** | Uses `libcurl` with TLS verification enforced |
| **SHA-256 checksum verification** | Validates every downloaded archive via OpenSSL before installing |
| **Atomic installs with rollback** | Backs up the current version before each update; restores on failure |
| **Post-install scripts** | Optionally runs a shell script bundled inside each package archive |
| **Version tracking** | Stamps installed version to disk; skips packages already up-to-date |
| **Configurable polling** | Runs in a loop, checking for updates at a configurable interval |
| **Graceful shutdown** | Handles `SIGINT`/`SIGTERM` cleanly via `std::atomic` flag |
| **Structured logging** | Thread-safe logging to stdout + file with timestamps and severity levels |
| **Systemd integration** | Ships a `.service` file for deploying as a background daemon |

---

## Architecture

```
cpp-update-agent/
├── include/
│   ├── update_agent.h   # Orchestrator — ties all components together
│   ├── config.h         # Reads agent.conf (key=value format)
│   ├── package.h        # Plain struct: name, version, url, checksum, script
│   ├── downloader.h     # HTTP/HTTPS download via libcurl
│   ├── verifier.h       # SHA-256 file verification via OpenSSL EVP API
│   ├── installer.h      # Extract tarball, run script, rollback
│   ├── logger.h         # Thread-safe logger (stdout + file)
│   └── nlohmann/
│       └── json.hpp     # Single-header JSON parser (downloaded at build time)
├── src/
│   ├── main.cpp         # Entry point, signal handling, poll loop
│   ├── update_agent.cpp # Core update cycle: parallel downloads + serial installs
│   ├── downloader.cpp
│   └── installer.cpp
├── config/
│   ├── agent.conf               # Runtime configuration
│   └── manifest.example.json    # Example remote package manifest
├── tests/
│   ├── test_verifier.cpp        # SHA-256: correct hash, wrong hash, missing file
│   ├── test_config.cpp          # Config: full parse, defaults, dry_run, whitespace
│   ├── test_manifest_parser.cpp # JSON: standard, multi-package, incomplete, invalid, empty
│   └── test_parallel_download.cpp # Concurrency: all complete, parallel faster than serial
├── scripts/
│   ├── build.sh                 # Local build helper
│   └── update_agent.service     # Systemd unit file
├── .github/workflows/ci.yml     # GitHub Actions CI — build + test on every push
└── CMakeLists.txt
```

---

## Update Cycle

Each poll cycle runs in two phases:

```
Download manifest.json
        │
        ▼
For each package — is installed version == manifest version?
    YES → skip
    NO  → queue for update
        │
        ▼
Phase 1 — Parallel (std::async):
    ├── Download tarball (libcurl, TLS)
    └── Verify SHA-256 (OpenSSL)
            │ all downloads complete
            ▼
Phase 2 — Serial (filesystem safety):
    ├── Backup current install
    ├── Extract + run install script
    │       ├── OK   → write .version stamp
    │       └── FAIL → rollback from backup
    └── Report results
```

**Why parallel downloads but serial installs?**  
Downloads are independent I/O operations on separate temp files — safe to parallelize.  
Installs write to shared directories (`install_dir`, `backup_dir`, version stamps) — concurrent writes would cause race conditions, so they run serially.

---

## Prerequisites

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install -y cmake g++ libcurl4-openssl-dev libssl-dev
```

**macOS:**
```bash
brew install cmake curl openssl
```

---

## Build

```bash
git clone https://github.com/Aishwaryaq27/cpp-update-agent.git
cd cpp-update-agent

# Download nlohmann/json (single header, not committed to repo)
mkdir -p include/nlohmann
curl -fsSL \
  https://raw.githubusercontent.com/nlohmann/json/develop/single_include/nlohmann/json.hpp \
  -o include/nlohmann/json.hpp

# Linux
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# macOS (Homebrew paths needed)
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DOPENSSL_ROOT_DIR=$(brew --prefix openssl) \
  -DCURL_ROOT=$(brew --prefix curl)
cmake --build build --parallel
```

---

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

Expected output:
```
Test 1/4 — verifier_test          Passed
Test 2/4 — config_test            Passed
Test 3/4 — manifest_parser_test   Passed
Test 4/4 — parallel_download_test Passed
```

### What each test covers

**test_verifier** — correct SHA-256 accepted, wrong hash rejected, missing file handled gracefully.

**test_config** — all config keys parsed, defaults applied when file is missing, `dry_run` flag, whitespace trimming.

**test_manifest_parser** — standard manifest, multiple packages, incomplete entries skipped, invalid JSON returns empty list, empty array handled.

**test_parallel_download** — all async tasks complete, parallel execution finishes faster than the equivalent serial time, output files are created correctly.

---

## Run the Agent

```bash
# Edit config first
nano config/agent.conf

./build/update_agent config/agent.conf
```

---

## Local Demo (without a real server)

```bash
# 1. Create a dummy package
mkdir -p /tmp/myapp-2.1.0
echo '#!/bin/bash
echo "myapp installed successfully!"' > /tmp/myapp-2.1.0/install.sh
tar -czf /tmp/agent_server/myapp-2.1.0.tar.gz -C /tmp myapp-2.1.0

# 2. Get its checksum
shasum -a 256 /tmp/agent_server/myapp-2.1.0.tar.gz

# 3. Write manifest.json with the checksum
mkdir -p /tmp/agent_server
# (edit manifest — see config/manifest.example.json for format)

# 4. Start a local HTTP server
cd /tmp && python3 -m http.server 8888

# 5. Point agent.conf at the local server and run
./build/update_agent config/agent.conf
```

---

## Deploy as a systemd Service (Linux)

```bash
sudo cmake --install build
sudo mkdir -p /etc/update_agent
sudo cp config/agent.conf /etc/update_agent/
sudo cp scripts/update_agent.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now update_agent
sudo journalctl -u update_agent -f
```

---

## Manifest Format

Host a `manifest.json` at any HTTPS URL:

```json
[
    {
        "name": "myapp",
        "version": "2.1.0",
        "url": "https://example.com/packages/myapp-2.1.0.tar.gz",
        "checksum": "sha256hexstring...",
        "install_script": "install.sh"
    }
]
```

Generate a checksum:
```bash
# Linux
sha256sum myapp-2.1.0.tar.gz

# macOS
shasum -a 256 myapp-2.1.0.tar.gz
```

---

## Configuration Reference

| Key | Default | Description |
|---|---|---|
| `manifest_url` | _(empty)_ | Remote URL to `manifest.json` |
| `download_dir` | `/tmp/update_agent/downloads` | Temp directory for downloads |
| `install_dir` | `/tmp/update_agent/installed` | Where packages are extracted |
| `backup_dir` | `/tmp/update_agent/backups` | Previous version backups |
| `poll_interval_seconds` | `60` | How often to check for updates |
| `timeout_seconds` | `30` | HTTP download timeout |
| `dry_run` | `false` | Simulate without installing |

---

## Technologies Used

| Technology | Purpose |
|---|---|
| **C++17** | `std::filesystem`, `std::async`, `std::future`, `std::atomic` |
| **libcurl** | HTTP/HTTPS downloads with TLS peer verification |
| **OpenSSL (EVP API)** | SHA-256 checksum verification |
| **nlohmann/json** | Single-header JSON manifest parser |
| **std::async / std::future** | Parallel download phase |
| **CMake** | Cross-platform build system with `find_package` |
| **GitHub Actions** | CI/CD: build + test on every push to `main` |
| **systemd** | Daemon deployment on Linux |

---

## Design Decisions

**Why nlohmann/json over cJSON or RapidJSON?**  
cJSON is a C library with manual memory management — a poor fit for a C++ project. RapidJSON is faster but has a verbose API. nlohmann/json is a single header file with a clean modern C++ API (`json["key"]` just works), battle-tested with 40k+ GitHub stars, and for a manifest file that's a few KB fetched every 30 seconds, correctness and developer ergonomics outweigh raw throughput.

**Why parallel downloads but serial installs?**  
Downloads are stateless I/O on separate temp files — safe to parallelize with `std::async`. Installs mutate shared directories and version stamp files; concurrent writes would race. The two-phase design gets the speed benefit of parallelism while keeping filesystem state consistent.

**Why start with a hand-rolled JSON parser?**  
The initial goal was zero extra dependencies. The custom parser worked for controlled input but broke on indentation variations during testing — an expected tradeoff. Migrating to nlohmann/json resolved all edge cases in a few lines and added proper error handling via exceptions.

---

## License

MIT
