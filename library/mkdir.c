#include "sifsutils.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

// make a new directory within an existing volume
int SIFS_mkdir(const char *volumename, const char *pathname)
{
    if (volumename == NULL || pathname == NULL || strlen(pathname) == 0)
    {
        SIFS_errno = SIFS_EINVAL;
        return SIFS_FAILURE;
    }

    size_t dircount;
    char** dirnames = strsplit(pathname, SIFS_DIR_DELIMITER, &dircount);
    if (dirnames == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return SIFS_FAILURE;
    }
    // Check if attempting to make a directory with same name as root dir
    if (dircount == 0)
    {
        SIFS_errno = SIFS_EINVAL;
        freesplit(dirnames);
        return SIFS_FAILURE;
    }
    char* newdirname = dirnames[dircount - 1];
    // Check if attempting to make a directory with same name as root dir
    if ((strlen(newdirname) == 1 && *newdirname == '.') || strlen(newdirname) >= SIFS_MAX_NAME_LENGTH)
    {
        SIFS_errno = SIFS_EINVAL;
        freesplit(dirnames);
        return SIFS_FAILURE;
    }

    // Find the parent directory to make the directory in
    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dirblock = SIFS_getdir(volumename, dirnames, dircount - 1, &dirblockId);
    if (dirblock == NULL)
    {
        // SIFS_errno set in SIFS_getdir()
        freesplit(dirnames);
        return SIFS_FAILURE;
    }
    // Check if the directory has enough remaining entries to add a new directory
    if (dirblock->nentries >= SIFS_MAX_ENTRIES)
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_EMAXENTRY;
        return SIFS_FAILURE;
    }
    // Make sure that the directory has no entries with newdirname (file or directory)
    if (SIFS_hasentry(volumename, dirblock, newdirname))
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_EEXIST;
        return SIFS_FAILURE;
    }
    // Allocate a new directory block
    SIFS_BLOCKID newBlockId = SIFS_allocateblocks(volumename, 1, SIFS_DIR);
    // Check if the allocation was successful
    if (newBlockId == SIFS_ROOTDIR_BLOCKID)
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_ENOSPC;
        return SIFS_FAILURE;
    }
    // Update parent directory entries and modtime
    dirblock->modtime = time(NULL);
    dirblock->entries[dirblock->nentries++].blockID = newBlockId;
    // Get the new directory block and set its entries and modtime
    SIFS_DIRBLOCK* newBlock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, newBlockId);
    memcpy(newBlock->name, newdirname, strlen(newdirname) + 1);
    newBlock->modtime = dirblock->modtime;
    newBlock->nentries = 0;

    // Rewrite both directory blocks to the volume
    SIFS_updateblock(volumename, dirblockId, dirblock, 0);
    SIFS_updateblock(volumename, newBlockId, newBlock, 0);
    
    free(newBlock);
    free(dirblock);
    freesplit(dirnames);
    SIFS_errno = SIFS_EOK;
    return SIFS_SUCCESS;
}
