#include "sifsutils.h"
#include <string.h>
#include <time.h>

// remove an existing directory from an existing volume
int SIFS_rmdir(const char *volumename, const char *dirname)
{
    if (volumename == NULL || dirname == NULL || strlen(dirname) == 0)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }
    void* volume = SIFS_readvolume(volumename, NULL);
    if (volume == NULL)
    {
        return 1;
    }
    size_t count;
    char** result = strsplit(dirname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        free(volume);
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }

    // Get the parent directory
    SIFS_DIRBLOCK* dir = SIFS_getdir(volume, result, count - 1);
    if (dir == NULL)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    if (!SIFS_hasentry(volume, dir, result[count - 1]))
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    int index = -1;
    SIFS_DIRBLOCK* block = NULL;
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volume, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volume, dir->entries[i].blockID);
            if (strcmp(dirblock->name, result[count - 1]) == 0)
            {
                index = i;
                block = dirblock;
                break;
            }
        }
    }
    if (index == -1)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_ENOTDIR;
        return 1;
    }
    if (block->nentries > 0)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }
    SIFS_freeblocks(volume, dir->entries[index].blockID, 1);
    dir->modtime = time(NULL);
    for (int i = index; i < dir->nentries - 1; i++)
    {
        dir->entries[i] = dir->entries[i + 1];
    }
    dir->nentries--;

    freesplit(result);
    SIFS_rewritevolume(volumename, volume);
    free(volume);
    SIFS_errno = SIFS_EOK;
    return 0;
}