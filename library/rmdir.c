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

    size_t count;
    char** result = strsplit(dirname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }

    // Get the parent directory
    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count - 1, &dirblockId);
    if (dir == NULL)
    {
        freesplit(result);
        return 1;
    }
    if (!SIFS_hasentry(volumename, dir, result[count - 1]))
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    int index = -1;
    SIFS_DIRBLOCK* block = NULL;
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
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
        free(dir);
        SIFS_errno = SIFS_ENOTDIR;
        return 1;
    }
    if (block->nentries > 0)
    {
        freesplit(result);
        free(block);
        free(dir);
        SIFS_errno = SIFS_ENOTEMPTY;
        return 1;
    }
    SIFS_freeblocks(volumename, dir->entries[index].blockID, 1);
    dir->modtime = time(NULL);
    for (int i = index; i < dir->nentries - 1; i++)
    {
        dir->entries[i] = dir->entries[i + 1];
    }
    dir->nentries--;

    SIFS_updateblock(volumename, dirblockId, dir, 0);

    freesplit(result);
    free(dir);
    free(block);
    SIFS_errno = SIFS_EOK;
    return 0;
}
