#include "sifsutils.h"
#include <string.h>
#include <time.h>

// remove an existing directory from an existing volume
int SIFS_rmdir(const char *volumename, const char *pathname)
{
    if (volumename == NULL || pathname == NULL || strlen(pathname) == 0)
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
    if (count == 0)
    {
        // Invalid argument (tried to remove root directory?)
        freesplit(result);
        SIFS_errno = SIFS_EINVAL;
        return SIFS_FAILURE;
    }
    char* dirname = result[count - 1];

    // Get the parent directory
    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count - 1, &dirblockId);
    if (dir == NULL)
    {
        // SIFS_errno set in SIFS_getdir()
        freesplit(result);
        return SIFS_FAILURE;
    }
    // Check whether the parent directory has any entry named dirname (files or directories)
    if (!SIFS_hasentry(volumename, dir, dirname))
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_ENOENT;
        return SIFS_FAILURE;
    }
    int index = -1;
    SIFS_DIRBLOCK* block = NULL;
    // Search through all directory entries to find the directory
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            if (strcmp(dirblock->name, dirname) == 0)
            {
                // Found directory entry, record it and its index
                index = i;
                block = dirblock;
                break;
            }
            free(dirblock);
        }
    }
    // Failed to find directory entry
    // Therefore the entry we are trying to delete must be a file or we would've returned earlier
    if (index == -1)
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_ENOTDIR;
        return SIFS_FAILURE;
    }
    // Check whether the directory is empty
    if (block->nentries > 0)
    {
        freesplit(result);
        free(block);
        free(dir);
        SIFS_errno = SIFS_ENOTEMPTY;
        return SIFS_FAILURE;
    }
    // Free the directory block
    SIFS_freeblocks(volumename, dir->entries[index].blockID, 1);
    dir->modtime = time(NULL);
    // Update the directory entries, any entry to the right of the deleted directory needs to be shifted left by 1
    for (int i = index; i < dir->nentries - 1; i++)
    {
        dir->entries[i] = dir->entries[i + 1];
    }
    dir->nentries--;

    // Rewrite directory to volume
    SIFS_updateblock(volumename, dirblockId, dir, 0);

    freesplit(result);
    free(dir);
    free(block);
    SIFS_errno = SIFS_EOK;
    return SIFS_SUCCESS;
}
