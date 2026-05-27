#include "error.h"
#include "linux_clock.h"
#include "linux_file.h"

#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                                         \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            return 1;                                                                              \
        }                                                                                          \
    } while (0)

static int test_reads_text_fixture(void)
{
    static const char expected[] = "cpu  100 20 30 400 5 6 7 0 0 0\n"
                                   "cpu0 60 10 15 200 3 4 5 0 0 0\n"
                                   "cpu1 40 10 15 200 2 2 2 0 0 0\n";
    const char path[] = "tests/fixtures/linux/proc-stat.sample";
    SymonError error;
    char *contents = NULL;
    size_t length = 0;

    symon_error_clear(&error);
    REQUIRE(symon_linux_read_text_file(path, &contents, &length, &error));
    REQUIRE(length == strlen(expected));
    REQUIRE(strcmp(contents, expected) == 0);

    free(contents);
    return 0;
}

static int test_missing_file_reports_io_error(void)
{
    const char path[] = "tests/fixtures/linux/missing.sample";
    SymonError error;
    char *contents = NULL;
    size_t length = 0;

    symon_error_clear(&error);
    REQUIRE(!symon_linux_read_text_file(path, &contents, &length, &error));
    REQUIRE(contents == NULL);
    REQUIRE(error.code == SYMON_ERROR_IO);
    REQUIRE(error.system_errno != 0);

    return 0;
}

static int test_linux_clock_returns_timestamps(void)
{
    SymonTimestamp timestamp;
    SymonError error;

    symon_error_clear(&error);
    REQUIRE(symon_linux_clock_now(&timestamp, &error));
    REQUIRE(timestamp.realtime_ms > 0);
    REQUIRE(timestamp.monotonic_ms > 0);

    return 0;
}

int main(void)
{
    REQUIRE(test_reads_text_fixture() == 0);
    REQUIRE(test_missing_file_reports_io_error() == 0);
    REQUIRE(test_linux_clock_returns_timestamps() == 0);
    return 0;
}
