#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sifs.h"
#include "library/sifsext.h"
#include <sys/stat.h>

void usage(const char* progname)
{
    printf("Usage: %s program [args...]\n", progname);
    exit(EXIT_FAILURE);
}

int ls(const char* volume, int argc, char** argv)
{
    SIFS_ls(volume, argv[2]);
    return 0;
}

int mymkdir(const char* volume, int argc, char** argv)
{
    SIFS_mkdir(volume, argv[2]);
    SIFS_perror(NULL);
    return 0;
}

int rmdir(const char* volume, int argc, char** argv)
{
    SIFS_rmdir(volume, argv[2]);
    SIFS_perror(NULL);
    return 0;
}

int dirinfo(const char* volume, int argc, char** argv)
{
    char** entries;
    uint32_t nentries;
    time_t modtime;
    int result = SIFS_dirinfo(volume, argv[2], &entries, &nentries, &modtime);
    SIFS_perror(NULL);
    if (result == 0)
    {
        printf("Directory %s\n", argv[2]);
        for (int i = 0; i < nentries; i++)
        {
            printf("Entry: %s\n", entries[i]);
        }
    }
    return 0;
}

int writefile(const char* volume, int argc, char** argv)
{
    const char* filename = argv[2];
    struct stat fstat;
    if (stat(filename, &fstat) == 0)
    {
        size_t size = fstat.st_size;
        char* buffer = (char*)malloc(size);
        FILE* file = fopen(filename, "r");
        fread(buffer, 1, size, file);
        SIFS_writefile(volume, argv[3], buffer, size);
        SIFS_perror(NULL);
        free(buffer);
        fclose(file);
    }
    return 0;
}

int readfile(const char* volume, int argc, char** argv)
{
    void* data;
    size_t length;
    SIFS_readfile(volume, argv[2], &data, &length);
    SIFS_perror(NULL);
    for (int i = 0; i < length; i++)
    {
        printf("%c", ((char*)data)[i]);
    }
    printf("\n");
    return 0;
}

int fileinfo(const char* volume, int argc, char** argv)
{
    size_t length;
    time_t modtime;
    SIFS_fileinfo(volume, argv[2], &length, &modtime);
    SIFS_perror(NULL);
    printf("Length %li\n", length);
    return 0;
}

int rmfile(const char* volume, int argc, char** argv)
{
    SIFS_rmfile(volume, argv[2]);
    SIFS_perror(NULL);
    return 0;
}

int main(int argcount, char *argvalue[])
{
    if (argcount < 2)
    {
        usage(argvalue[0]);
    }
    const char* volume = "volume";
    SIFS_mkvolume(volume, 4096, 16);
    
    if (strcmp(argvalue[1], "ls") == 0)
    {
        return ls(volume, argcount, argvalue);
    }
    if (strcmp(argvalue[1], "mkdir") == 0)
    {
        return mymkdir(volume, argcount, argvalue);
    }
    if (strcmp(argvalue[1], "rmdir") == 0)
    {
        return rmdir(volume, argcount, argvalue);
    }
    if (strcmp(argvalue[1], "dirinfo") == 0)
    {
        return dirinfo(volume, argcount, argvalue);
    }
    if (strcmp(argvalue[1], "writefile") == 0)
    {
        return writefile(volume, argcount, argvalue);
    }
    if (strcmp(argvalue[1], "readfile") == 0)
    {
        return readfile(volume, argcount, argvalue);
    }
    if (strcmp(argvalue[1], "fileinfo") == 0)
    {
        return fileinfo(volume, argcount, argvalue);
    }
    if (strcmp(argvalue[1], "rmfile") == 0)
    {
        return rmfile(volume, argcount, argvalue);
    }

    return EXIT_SUCCESS;
}