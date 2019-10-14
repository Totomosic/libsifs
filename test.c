#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "sifs.h"
#include "library/sifs-internal.h"

void test_writefile_EINVAL(void)
{
    printf("TESTING writefile EINVAL\n");
    SIFS_mkvolume("volume", 1024, 32);
    bool passed = true;

    int data = 10;
    size_t datasize = sizeof(int);
    
    passed = passed && SIFS_writefile(NULL, NULL, NULL, 0) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile(NULL, NULL, NULL, 1000) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile(NULL, "File", NULL, 0) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile("volume", NULL, NULL, 0) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile("volume", "file", NULL, 0) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile("volume", "file", &data, 0) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile("volume", "", &data, datasize) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile("volume", "/", &data, datasize) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile("volume", "//////", &data, datasize) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_writefile("volume", "file", &data, datasize) == 0;
    
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

void test_writefile(void)
{
    printf("TESTING writefile\n");
    SIFS_mkvolume("volume", 1024, 32);
    bool passed = true;

    int data = 10;
    size_t datasize = sizeof(int);

    passed = passed && SIFS_writefile("volume", "file", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file1", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file2", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file3", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file4", &data, datasize) == 0;
    data = 100;
    passed = passed && SIFS_writefile("volume", "file5", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file6", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file7", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file8", &data, datasize) == 0;
    passed = passed && SIFS_writefile("volume", "file9", &data, datasize) == 0;

    void* dataPtr;
    size_t newDataSize;
    passed = passed && SIFS_readfile("volume", "file5", &dataPtr, &newDataSize) == 0;
    passed = passed && newDataSize == datasize && *(int*)dataPtr == data;
    
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

void test_writefile_ENOENT(void)
{
    printf("TESTING writefile ENOENT\n");
    SIFS_mkvolume("volume", 1024, 32);
    bool passed = true;

    int data = 10;
    size_t datasize = sizeof(int);
    
    passed = passed && SIFS_writefile("volume", "Dir/File", &data, datasize) == 1 && SIFS_errno == SIFS_ENOENT;
    passed = passed && SIFS_writefile("volume", "Dir/Dir/File", &data, datasize) == 1 && SIFS_errno == SIFS_ENOENT;
    
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

void test_writefile_EMAXENTRY(void)
{
    printf("TESTING writefile EMAXENTRY\n");
    SIFS_mkvolume("volume", 1024, 64);
    bool passed = true;

    int data = 10;
    size_t datasize = sizeof(int);
    
    passed = passed && SIFS_mkdir("volume", "test0/////") == 0;
    for (int i = 0; i < SIFS_MAX_ENTRIES; i++)
    {
        char filename[SIFS_MAX_NAME_LENGTH];
        sprintf(filename, "test0/File %i", i);
        passed = passed && SIFS_writefile("volume", filename, &data, datasize) == 0;
    }
    passed = passed && SIFS_writefile("volume", "test0/F", &data, datasize) == 1 && SIFS_errno == SIFS_EMAXENTRY;

    passed = passed && SIFS_mkdir("volume", "test1") == 0;
    for (int i = 0; i < SIFS_MAX_ENTRIES; i++)
    {
        data += 1;
        char filename[SIFS_MAX_NAME_LENGTH];
        sprintf(filename, "test1/File %i", i);
        passed = passed && SIFS_writefile("volume", filename, &data, datasize) == 0;
    }
    data += 1;
    passed = passed && SIFS_writefile("volume", "test1/F", &data, datasize) == 1 && SIFS_errno == SIFS_EMAXENTRY;
    
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
    passed = passed && SIFS_rmdir("volume", "/Dir//Dir1") == 1 && SIFS_errno == SIFS_ENOENT;
    passed = passed && SIFS_rmdir("volume", "///////////11.2.3") == 1 && SIFS_errno == SIFS_ENOENT;
    SIFS_mkdir("volume", "Dir");
    passed = passed && SIFS_rmdir("volume", "/Dir/") == 0;
    passed = passed && SIFS_rmdir("volume", "Dir/////") == 1 && SIFS_errno == SIFS_ENOENT;
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

void test_rmdir_ENOTDIR(void)
{
    printf("TESTING rmdir ENOTDIR\n");
    SIFS_mkvolume("volume", 1024, 32);
    bool passed = true;

    int data = 10;
    passed = passed && SIFS_writefile("volume", "file", &data, sizeof(int)) == 0;
    passed = passed && SIFS_rmdir("volume", "file") == 1 && SIFS_errno == SIFS_ENOTDIR;

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

void test_rmdir_EINVAL(void)
{
    printf("TESTING rmdir EINVAL\n");
    SIFS_mkvolume("volume", 1024, 32);
    bool passed = true;

    passed = passed && SIFS_rmdir(NULL, NULL) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_rmdir("", NULL) == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_rmdir("volume", "/") == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_rmdir("volume", "") == 1 && SIFS_errno == SIFS_EINVAL;
    passed = passed && SIFS_rmdir("volume", "//////////") == 1 && SIFS_errno == SIFS_EINVAL;

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

void test_rmdir_ENOTEMPTY(void)
{
    printf("TESTING rmdir ENOTEMPTY\n");
    SIFS_mkvolume("volume", 1024, 32);
    bool passed = true;

    passed = passed && SIFS_mkdir("volume", "Dir") == 0;
    passed = passed && SIFS_mkdir("volume", "Dir/Dir1") == 0;

    passed = passed && SIFS_rmdir("volume", "/Dir") == 1 && SIFS_errno == SIFS_ENOTEMPTY;
    passed = passed && SIFS_rmdir("volume", "/Dir/Dir1") == 0;
    passed = passed && SIFS_rmdir("volume", "Dir") == 0;

    char** entries;
    uint32_t entryCount;
    time_t modtime;
    passed = passed && SIFS_dirinfo("volume", "/", &entries, &entryCount, &modtime) == 0;
    passed = passed && entryCount == 0;
    
    for (int i = 0; i < entryCount; i++)
    {
        free(entries[i]);
    }
    free(entries);

    int data = 10;
    passed = passed && SIFS_mkdir("volume", "///Dir") == 0;
    passed = passed && SIFS_writefile("volume", "Dir/File", &data, sizeof(int)) == 0;
    passed = passed && SIFS_rmdir("volume", "Dir") == 1 && SIFS_errno == SIFS_ENOTEMPTY;

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

void check_failure(bool passed, const char* message)
{
    if (!passed)
    {
        printf("%s\n", message);
        SIFS_perror(NULL);
    }
}

void free_entries(char** entries, uint32_t nentries)
{
    if (entries == NULL)
    {
        return;
    }
    for (int i = 0; i < nentries; i++)
    {
        free(entries[i]);
    }
    free(entries);
}

void test_random(void)
{
    SIFS_mkvolume("volume", 1024, 128);
    bool passed = true;

    passed = passed && SIFS_mkdir("volume", "Dir") == 0;
    check_failure(passed, "mkdir \"Dir\" failed");
    passed = passed && SIFS_mkdir("volume", "Dir1") == 0;
    check_failure(passed, "mkdir \"Dir1\" failed");
    passed = passed && SIFS_mkdir("volume", "Dir2") == 0;
    check_failure(passed, "mkdir \"Dir2\" failed");

    passed = passed && SIFS_mkdir("volume", "Dir/SubDir") == 0;
    check_failure(passed, "mkdir \"Dir/SubDir\" failed");
    passed = passed && SIFS_rmfile("volume", "Dir") == 1 && SIFS_errno == SIFS_ENOTFILE;
    check_failure(passed, "rmfile \"Dir\" failed");
    passed = passed && SIFS_rmdir("volume", "Dir") == 1 && SIFS_errno == SIFS_ENOTEMPTY;
    check_failure(passed, "rmdir \"Dir\" failed");

    passed = passed && SIFS_defrag("volume") == 0;
    check_failure(passed, "first defrag failed");
    passed = passed && SIFS_defrag("volume") == 0;
    check_failure(passed, "second defrag failed");

    passed = passed && SIFS_rmdir("volume", "Dir1") == 0;
    check_failure(passed, "rmdir \"Dir1\" failed");

    passed = passed && SIFS_defrag("volume") == 0;
    check_failure(passed, "third defrag failed");

    char** entries;
    uint32_t nentries;
    time_t modtime;
    passed = passed && SIFS_dirinfo("volume", "Dir", &entries, &nentries, &modtime) == 0;
    check_failure(passed, "dirinfo \"Dir\" failed");
    passed = passed && nentries == 1 && strcmp(entries[0], "SubDir") == 0;
    check_failure(passed, "invalid dirinfo for \"Dir\"");
    free_entries(entries, nentries);

    size_t data = 100;
    passed = passed && SIFS_writefile("volume", "Dir/SubDir/File", &data, sizeof(data)) == 0;
    check_failure(passed, "failed to write integer");

    passed = passed && SIFS_fileinfo("volum", "Dir/SubDir/File", &data, &modtime) == 1 && SIFS_errno == SIFS_ENOVOL;
    check_failure(passed, "error not caught on fileinfo");
    passed = passed && data == 100;
    check_failure(passed, "data was modified on failed fileinfo");

    passed = passed && SIFS_fileinfo("volume", "Dir/SubDir/File", &data, &modtime) == 0;
    check_failure(passed, "failed to get fileinfo");
    passed = passed && data == sizeof(data);
    check_failure(passed, "invalid file length");
    void* dataPtr;
    passed = passed && SIFS_readfile("volume", "Dir/SubDir/File", &dataPtr, &data) == 0;
    check_failure(passed, "Failed to read file");
    passed = passed && *(size_t*)dataPtr == 100;
    check_failure(passed, "Corrupted file contents");

    passed = passed && SIFS_rmfile("volume", "Dir/SubDir/File") == 0;
    check_failure(passed, "failed to remove file");
    passed = passed && SIFS_defrag("volume") == 0;
    check_failure(passed, "failed fourth defrag");

    passed = passed && SIFS_dirinfo("volume", "Dir//", &entries, &nentries, &modtime) == 0;
    check_failure(passed, "Failed to get dirinfo");
    passed = passed && nentries == 1;
    check_failure(passed, "Invalid nentries");
    
    free_entries(entries, nentries);

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
    printf("TESTING writefile\n");
    test_writefile_EINVAL();
    test_writefile_ENOENT();
    test_writefile_EMAXENTRY();
    test_writefile();

    printf("TESTING rmdir\n");
    test_rmdir_ENOVOL();
    test_rmdir_ENOTVOL();
    test_rmdir_ENOENT();
    test_rmdir_ENOTDIR();
    test_rmdir_EINVAL();
    test_rmdir_ENOTEMPTY();

    printf("RANDOM TESTS\n");
    test_random();
    return 0;
}