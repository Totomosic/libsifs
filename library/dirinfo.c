#include "sifsutils.h"
#include <stdio.h>
#include <string.h>

// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
{
    if (volumename == NULL || pathname == NULL)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    size_t count;
    char** result = strsplit(pathname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }

    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count, NULL);
    if (dir == NULL)
    {
        freesplit(result);
        return 1;
    }
    *nentries = dir->nentries;
    *modtime = dir->modtime;
    char** entries = (char**)malloc(sizeof(char*) * dir->nentries);
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            size_t length = strlen(dirblock->name);
            entries[i] = (char*)malloc(length + 1);
            memcpy(entries[i], dirblock->name, length + 1);
            free(dirblock);
        }
        else
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            char* filename = fileblock->filenames[dir->entries[i].fileindex];
            size_t length = strlen(filename);
            entries[i] = (char*)malloc(length + 1);
            memcpy(entries[i], filename, length + 1);
            free(fileblock);
        }
    }
    *entrynames = entries;

    free(dir);
    freesplit(result);
    SIFS_errno = SIFS_EOK;
    return 0;
}
