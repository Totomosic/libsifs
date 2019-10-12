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
        return SIFS_FAILURE;
    }

    size_t count;
    char** result = strsplit(pathname, SIFS_DIR_DELIMITER, &count);
    if (result == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return SIFS_FAILURE;
    }

    // Find the directory referenced to by pathname
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count, NULL);
    if (dir == NULL)
    {
        // SIFS_errno set in SIFS_getdir()
        freesplit(result);
        return SIFS_FAILURE;
    }
    *nentries = dir->nentries;
    *modtime = dir->modtime;
    // Create entries vector
    char** entries = (char**)malloc(sizeof(char*) * dir->nentries);
    // Iterate through each entry in the directory
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            // Found a directory entry, add its name to the list of entries
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            size_t length = strlen(dirblock->name);
            entries[i] = (char*)malloc(length + 1);
            memcpy(entries[i], dirblock->name, length + 1);
            free(dirblock);
        }
        else
        {
            // Found a file entry, add the correct filename to the list of entries
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
    return SIFS_SUCCESS;
}
