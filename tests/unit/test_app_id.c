#include "symon_constants.h"

#include <string.h>

int main(void)
{
    const char *expected_id = "io.github.symon.SyMon";
    const char *expected_name = "SyMon";

    if (strcmp(SYMON_APPLICATION_ID, expected_id) != 0) {
        return 1;
    }

    if (strcmp(SYMON_APPLICATION_NAME, expected_name) != 0) {
        return 1;
    }

    return 0;
}
