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
        return SIFS_ERROR;
    }

    size_t dircount;
    char** dirnames = strsplit(pathname, SIFS_DIR_DELIMETER, &dircount);
    if (dirnames == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return SIFS_ERROR;
    }
    if (dircount == 0)
    {
        SIFS_errno = SIFS_EINVAL;
        freesplit(dirnames);
        return SIFS_ERROR;
    }
    char* newdirname = dirnames[dircount - 1];
    if ((strlen(newdirname) == 1 && *newdirname == '.') || strlen(newdirname) >= SIFS_MAX_NAME_LENGTH)
    {
        SIFS_errno = SIFS_EINVAL;
        freesplit(dirnames);
        return SIFS_ERROR;
    }

    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dirblock = SIFS_getdir(volumename, dirnames, dircount - 1, &dirblockId);
    if (dirblock == NULL)
    {
        freesplit(dirnames);
        return SIFS_ERROR;
    }
    if (dirblock->nentries >= SIFS_MAX_ENTRIES)
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_EMAXENTRY;
        return SIFS_ERROR;
    }
    if (SIFS_hasentry(volumename, dirblock, newdirname))
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_EEXIST;
        return SIFS_ERROR;
    }
    SIFS_BLOCKID newBlockId = SIFS_allocateblocks(volumename, 1, SIFS_DIR);
    if (newBlockId == SIFS_ROOTDIR_BLOCKID)
    {
        freesplit(dirnames);
        free(dirblock);
        SIFS_errno = SIFS_ENOSPC;
        return SIFS_ERROR;
    }
    dirblock->modtime = time(NULL);
    dirblock->entries[dirblock->nentries++].blockID = newBlockId;
    SIFS_DIRBLOCK* newBlock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, newBlockId);
    memcpy(newBlock->name, newdirname, strlen(newdirname) + 1);
    newBlock->modtime = dirblock->modtime;
    newBlock->nentries = 0;

    SIFS_updateblock(volumename, dirblockId, dirblock, 0);
    SIFS_updateblock(volumename, newBlockId, newBlock, 0);
    
    free(newBlock);
    free(dirblock);
    freesplit(dirnames);
    SIFS_errno = SIFS_EOK;
    return SIFS_OK;
}
