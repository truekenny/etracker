#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "exit_code.h"

void exitPrint(int exitCode, char *file, char printError) {
    if (printError == EXIT_CODE_PRINT_ERROR_YES)
        printf("exit:%d; file:%s; errno:%d; text:%s\n", exitCode, file, errno, strerror(errno));
    else
        printf("exit:%d; file:%s\n", exitCode, file);

    exit(exitCode);
}