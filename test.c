#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "sifs.h"

void test_rmdir_ENOVOL(void)
{
    printf("TESTING rmdir ENOVOL\n");
    if (SIFS_rmdir("NOT_A_VOLUME", "Dir") == 1 && SIFS_errno == SIFS_ENOVOL)
    {
        printf("TEST PASSED\n");
    }
    else
    {
        printf("TEST FAILED\n");
        SIFS_perror(NULL);
    }
}

void test_rmdir_ENOTVOL(void)
{
    printf("TESTING rmdir ENOTVOL\n");
    if (SIFS_rmdir("test.c", "Dir") == 1 && SIFS_errno == SIFS_ENOTVOL)
    {
        printf("TEST PASSED\n");
    }
    else
    {
        SIFS_perror(NULL);
        printf("TEST FAILED\n");
    }    
}

void test_rmdir_ENOENT(void)
{
    printf("TESTING rmdir ENOENT\n");
    SIFS_mkvolume("volume", 1024, 16);
    bool passed = true;
    passed = passed && SIFS_rmdir("volume", "Dir") == 1 && SIFS_errno == SIFS_ENOENT;
    passed = passed && SIFS_rmdir("volume", "Dir/Dir1") == 1 && SIFS_errno == SIFS_ENOENT;
    passed = passed && SIFS_rmdir("volume", "11.2.3") == 1 && SIFS_errno == SIFS_ENOENT;
    SIFS_mkdir("volume", "Dir");
    passed = passed && SIFS_rmdir("volume", "Dir") == 0;
    passed = passed && SIFS_rmdir("volume", "Dir") == 1 && SIFS_errno == SIFS_ENOENT;
    if (passed)
    {
        printf("TEST PASSED\n");
    }
    else
    {
        printf("TEST FAILED\n");
    }
    remove("volume");
}

int main(int argc, char** argv)
{
    printf("TESTING rmdir\n");
    test_rmdir_ENOVOL();
    test_rmdir_ENOTVOL();
    test_rmdir_ENOENT();
    return 0;
}