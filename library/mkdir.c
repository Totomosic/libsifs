#include "sifsutils.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

// make a new directory within an existing volume
int SIFS_mkdir(const char *volumename, const char *dirname)
{
    if (volumename == NULL || dirname == NULL || strlen(dirname) == 0 || strlen(dirname) >= SIFS_MAX_NAME_LENGTH)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    size_t dircount;
    char** dirnames = strsplit(dirname, SIFS_DIR_DELIMETER, &dircount);
    if (dirnames == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }

    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dirblock = SIFS_getdir(volumename, dirnames, dircount - 1, &dirblockId);
    if (dirblock == NULL)
    {
        freesplit(dirnames);
        return 1;
    }
    if (dirblock->nentries >= SIFS_MAX_ENTRIES)
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }
    if (SIFS_hasentry(volumename, dirblock, dirnames[dircount - 1]))
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_EEXIST;
        return 1;
    }
    SIFS_BLOCKID newBlockId = SIFS_allocateblocks(volumename, 1, SIFS_DIR);
    if (newBlockId == SIFS_ROOTDIR_BLOCKID)
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_ENOSPC;
        return 1;
    }
    dirblock->modtime = time(NULL);
    dirblock->entries[dirblock->nentries++].blockID = newBlockId;
    SIFS_DIRBLOCK* newBlock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, newBlockId);
    memcpy(newBlock->name, dirnames[dircount - 1], strlen(dirnames[dircount - 1]) + 1);
    newBlock->modtime = dirblock->modtime;
    newBlock->nentries = 0;

    SIFS_updateblock(volumename, dirblockId, dirblock, 0);
    SIFS_updateblock(volumename, newBlockId, newBlock, 0);
    
    free(newBlock);
    free(dirblock);
    freesplit(dirnames);
    SIFS_errno = SIFS_EOK;
    return 0;
}
