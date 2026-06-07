cmake_minimum_required(VERSION 3.16)
project(cpp_update_agent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ----------- Dependencies -----------
find_package(CURL    REQUIRED)
find_package(OpenSSL REQUIRED)

# Threads (needed for std::async / std::future on Linux)
find_package(Threads REQUIRED)

# ----------- Sources ----------------
set(SOURCES
    src/main.cpp
    src/update_agent.cpp
    src/downloader.cpp
    src/installer.cpp
)

# ----------- Executable -------------
add_executable(update_agent ${SOURCES})

target_include_directories(update_agent PRIVATE include)

target_link_libraries(update_agent PRIVATE
    CURL::libcurl
    OpenSSL::Crypto
    Threads::Threads
)

target_compile_options(update_agent PRIVATE
    -Wall -Wextra -Wpedantic -O2
)

# ----------- Install ----------------
install(TARGETS update_agent DESTINATION bin)
install(FILES config/agent.conf DESTINATION etc/update_agent)

# ----------- Tests ------------------
enable_testing()

# Test 1: Verifier (SHA-256)
add_executable(test_verifier tests/test_verifier.cpp)
target_include_directories(test_verifier PRIVATE include)
target_link_libraries(test_verifier PRIVATE OpenSSL::Crypto)
target_compile_options(test_verifier PRIVATE -Wall -Wextra -O2)
add_test(NAME verifier_test COMMAND test_verifier)

# Test 2: Config parser
add_executable(test_config tests/test_config.cpp)
target_include_directories(test_config PRIVATE include)
target_compile_options(test_config PRIVATE -Wall -Wextra -O2)
add_test(NAME config_test COMMAND test_config)

# Test 3: Manifest JSON parser (nlohmann/json)
add_executable(test_manifest_parser tests/test_manifest_parser.cpp)
target_include_directories(test_manifest_parser PRIVATE include)
target_compile_options(test_manifest_parser PRIVATE -Wall -Wextra -O2)
add_test(NAME manifest_parser_test COMMAND test_manifest_parser)

# Test 4: Parallel download logic
add_executable(test_parallel_download tests/test_parallel_download.cpp)
target_include_directories(test_parallel_download PRIVATE include)
target_link_libraries(test_parallel_download PRIVATE Threads::Threads)
target_compile_options(test_parallel_download PRIVATE -Wall -Wextra -O2)
add_test(NAME parallel_download_test COMMAND test_parallel_download)
