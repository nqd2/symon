# SyMon

SyMon is a lightweight, native Linux system monitor and process manager written in C23 with
GTK4.

The project targets Ubuntu 24.04 and a small desktop footprint: low idle CPU usage, low memory
usage, and no background service or telemetry.

## Current Status

The bootstrap milestone provides a GTK4 application shell, CMake build, application metadata,
quality tooling configuration, tests, and CI. Metric collectors and process management follow in
later milestones.

## Build

Install dependencies on Ubuntu 24.04:

```bash
sudo apt install build-essential cmake libgtk-4-dev pkg-config
```

Configure, build, test, and launch:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
./build/symon
```

## Quality Checks

```bash
sudo apt install clang-format clang-tidy cppcheck
clang-format --dry-run --Werror $(git ls-files '*.[ch]')
clang-tidy -p build src/app/*.c tests/unit/*.c
cppcheck --enable=warning,style,performance,portability --error-exitcode=1 \
  --suppress=missingIncludeSystem src/ tests/
```

Sanitizer build:

```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DSYMON_ENABLE_ASAN=ON -DSYMON_ENABLE_UBSAN=ON
cmake --build build-asan
ctest --test-dir build-asan --output-on-failure
```

## Privacy

SyMon will not include telemetry. It will write no local logs unless the user enables debug
output. Optional SQLite history remains disabled by default.

## License

SyMon is licensed under GPL-3.0. See [LICENSE](LICENSE).
