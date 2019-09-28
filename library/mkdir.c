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
    size_t volumeSize;
    void* volume = SIFS_readvolume(volumename, &volumeSize);
    if (volume == NULL)
    {
        return 1;
    }

    size_t dircount;
    char** dirnames = strsplit(dirname, SIFS_DIR_DELIMETER, &dircount);
    if (dirnames == NULL)
    {
        free(volume);
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }

    SIFS_DIRBLOCK* dirblock = SIFS_getdir(volume, dirnames, dircount - 1);
    if (dirblock == NULL)
    {
        freesplit(dirnames);
        free(volume);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    if (dirblock->nentries >= SIFS_MAX_ENTRIES)
    {
        freesplit(dirnames);
        free(volume);
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }
    if (SIFS_hasentry(volume, dirblock, dirnames[dircount - 1]))
    {
        freesplit(dirnames);
        free(volume);
        SIFS_errno = SIFS_EEXIST;
        return 1;
    }
    SIFS_BLOCKID dirblockId = SIFS_allocateblocks(volume, 1, SIFS_DIR);
    if (dirblockId == SIFS_ROOTDIR_BLOCKID)
    {
        freesplit(dirnames);
        free(volume);
        SIFS_errno = SIFS_ENOSPC;
        return 1;
    }
    dirblock->modtime = time(NULL);
    dirblock->entries[dirblock->nentries++].blockID = dirblockId;
    SIFS_DIRBLOCK* block = (SIFS_DIRBLOCK*)SIFS_getblock(volume, dirblockId);
    memcpy(block->name, dirnames[dircount - 1], strlen(dirnames[dircount - 1]) + 1);
    block->modtime = dirblock->modtime;
    block->nentries = 0;
    freesplit(dirnames);
    SIFS_rewritevolume(volumename, volume);
    free(volume);
    SIFS_errno = SIFS_EOK;
    return 0;
}
