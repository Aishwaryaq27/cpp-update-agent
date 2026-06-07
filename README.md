# cpp-update-agent

A reliable, small-scale **software update/patch agent** written in modern C++17 for Linux.  
Built for learning, portfolios, and real-world use.

---

## Features

| Feature | Details |
|---|---|
| **Manifest-driven updates** | Fetches a JSON manifest from a remote URL listing packages with versions and download URLs |
| **Secure downloads** | Uses `libcurl` with TLS verification enforced |
| **SHA-256 checksum verification** | Validates every downloaded archive via OpenSSL before installing |
| **Atomic installs with rollback** | Backs up the current version before each update; restores on failure |
| **Post-install scripts** | Optionally runs a shell script bundled inside each package archive |
| **Version tracking** | Stamps installed version to disk; skips packages already up-to-date |
| **Configurable polling** | Runs in a loop, checking for updates at a configurable interval |
| **Graceful shutdown** | Handles `SIGINT`/`SIGTERM` cleanly |
| **Structured logging** | Logs to stdout + file with timestamps and severity levels |
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
│   ├── verifier.h       # SHA-256 file verification via OpenSSL
│   ├── installer.h      # Extract tarball, run script, rollback
│   └── logger.h         # Thread-safe logger (stdout + file)
├── src/
│   ├── main.cpp         # Entry point, signal handling, poll loop
│   ├── update_agent.cpp # Core update cycle logic
│   ├── downloader.cpp
│   └── installer.cpp
├── config/
│   ├── agent.conf               # Runtime configuration
│   └── manifest.example.json    # Example remote package manifest
├── tests/
│   ├── test_verifier.cpp
│   └── test_config.cpp
├── scripts/
│   ├── build.sh                 # Local build helper
│   └── update_agent.service     # Systemd unit file
├── .github/workflows/ci.yml     # GitHub Actions CI
└── CMakeLists.txt
```

### Update Cycle (per poll interval)

```
Download manifest.json
        │
        ▼
For each package in manifest:
    Is installed version == manifest version?
        YES → skip
        NO  →
            ├── Download tarball (libcurl, TLS)
            ├── Verify SHA-256 (OpenSSL)
            ├── Backup current install
            ├── Extract + run install script
            │       ├── OK  → write .version stamp
            │       └── FAIL → rollback from backup
            └── Report results
```

---

## Prerequisites

```bash
sudo apt-get install -y cmake g++ libcurl4-openssl-dev libssl-dev
```

---

## Build

```bash
git clone https://github.com/YOUR_USERNAME/cpp-update-agent.git
cd cpp-update-agent
chmod +x scripts/build.sh
./scripts/build.sh         # Release build
./scripts/build.sh Debug   # Debug build
```

Or manually:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## Run

```bash
# Edit config first
nano config/agent.conf

# Run the agent (polls every N seconds as configured)
./build/update_agent config/agent.conf
```

---

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

---

## Deploy as a systemd Service

```bash
# Install binary and config
sudo cmake --install build
sudo mkdir -p /etc/update_agent
sudo cp config/agent.conf /etc/update_agent/

# Install and start the service
sudo cp scripts/update_agent.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now update_agent

# Check status and logs
sudo systemctl status update_agent
sudo journalctl -u update_agent -f
```

---

## Manifest Format

Host a `manifest.json` at any HTTP/HTTPS URL:

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
sha256sum myapp-2.1.0.tar.gz
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

- **C++17** — `std::filesystem`, structured bindings, `if constexpr`
- **libcurl** — HTTP/HTTPS downloads with TLS
- **OpenSSL (EVP API)** — SHA-256 checksum verification
- **CMake** — Cross-project build system
- **GitHub Actions** — CI/CD: build + test on every push
- **systemd** — Daemon deployment on Linux

---

## License

MIT
