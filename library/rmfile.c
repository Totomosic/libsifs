#include "sifsutils.h"
#include <string.h>
#include <stdio.h>

// remove an existing file from an existing volume
int SIFS_rmfile(const char *volumename, const char *pathname)
{
    if (volumename == NULL || pathname == NULL || strlen(pathname) == 0)
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
    char** result = strsplit(pathname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        free(volume);
        SIFS_errno = SIFS_ENOMEM;
        return 11;
    }
    if (count == 0)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    char* filename = result[count - 1];
    SIFS_DIRBLOCK* dir = SIFS_getdir(volume, result, count - 1);
    if (dir == NULL)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    
    if (!SIFS_hasentry(volume, dir, filename))
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    SIFS_BLOCKID blockId = SIFS_ROOTDIR_BLOCKID;
    int entryId = -1;
    int fileIndex = -1;
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volume, dir->entries[i].blockID);
        if (type == SIFS_FILE)
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volume, dir->entries[i].blockID);
            if (strcmp(fileblock->filenames[dir->entries[i].fileindex], filename) == 0)
            {
                entryId = i;
                blockId = dir->entries[i].blockID;
                fileIndex = dir->entries[i].fileindex;
                break;
            }
        }
    }
    if (entryId == -1)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_ENOTFILE;
        return 1;
    }
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volume);
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volume);
    for (SIFS_BLOCKID i = 0; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volume, i);
            for (int i = 0; i < dirblock->nentries; i++)
            {
                if (dirblock->entries[i].blockID == blockId && dirblock->entries[i].fileindex > fileIndex)
                {
                    dirblock->entries[i].fileindex--;
                }
            }
        }
    }
    SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volume, blockId);
    for (int i = fileIndex; i < fileblock->nfiles - 1; i++)
    {
        memcpy(fileblock->filenames[i], fileblock->filenames[i + 1], SIFS_MAX_NAME_LENGTH);
    }
    fileblock->nfiles--;
    if (fileblock->nfiles <= 0)
    {
        int nblocks = fileblock->length / header->blocksize + ((fileblock->length % header->blocksize == 0) ? 0 : 1);
        SIFS_freeblocks(volume, fileblock->firstblockID, nblocks);
        SIFS_freeblocks(volume, blockId, 1);
    }
    for (int i = entryId; i < dir->nentries - 1; i++)
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
