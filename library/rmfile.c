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

    size_t count;
    char** result = strsplit(pathname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }
    if (count == 0)
    {
        freesplit(result);
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    char* filename = result[count - 1];
    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count - 1, &dirblockId);
    if (dir == NULL)
    {
        freesplit(result);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    
    if (!SIFS_hasentry(volumename, dir, filename))
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_ENOENT;
        return 1;
    }
    SIFS_BLOCKID blockId = SIFS_ROOTDIR_BLOCKID;
    int entryId = -1;
    int fileIndex = -1;
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_FILE)
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            if (strcmp(fileblock->filenames[dir->entries[i].fileindex], filename) == 0)
            {
                entryId = i;
                blockId = dir->entries[i].blockID;
                fileIndex = dir->entries[i].fileindex;
                free(fileblock);
                break;
            }
            free(fileblock);
        }
    }
    if (entryId == -1)
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_ENOTFILE;
        return 1;
    }
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    for (SIFS_BLOCKID i = 0; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, i);
            for (int j = 0; j < dirblock->nentries; j++)
            {
                if (i != dirblockId && dirblock->entries[j].blockID == blockId && dirblock->entries[j].fileindex > fileIndex)
                {
                    dirblock->entries[j].fileindex--;
                }
            }
            SIFS_updateblock(volumename, i, dirblock, 0);
            free(dirblock);
        }
    }
    for (int j = 0; j < dir->nentries; j++)
    {
        if (dir->entries[j].blockID == blockId && dir->entries[j].fileindex > fileIndex)
        {
            dir->entries[j].fileindex--;
        }
    }
    SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, blockId);
    for (int i = fileIndex; i < fileblock->nfiles - 1; i++)
    {
        memcpy(fileblock->filenames[i], fileblock->filenames[i + 1], SIFS_MAX_NAME_LENGTH);
    }
    // Clear the last name
    memset(fileblock->filenames[fileblock->nfiles - 1], 0, SIFS_MAX_NAME_LENGTH);
    fileblock->nfiles--;

    if (fileblock->nfiles <= 0)
    {
        int nblocks = fileblock->length / header->blocksize + ((fileblock->length % header->blocksize == 0) ? 0 : 1);
        SIFS_freeblocks(volumename, fileblock->firstblockID, nblocks);
        SIFS_freeblocks(volumename, blockId, 1);
    }
    SIFS_updateblock(volumename, blockId, fileblock, 0);
    free(fileblock);
    for (int i = entryId; i < dir->nentries - 1; i++)
    {
        dir->entries[i] = dir->entries[i + 1];
    }
    dir->nentries--;
    SIFS_updateblock(volumename, dirblockId, dir, 0);

    freesplit(result);
    free(dir);
    free(header);
    free(bitmap);
    SIFS_errno = SIFS_EOK;
    return 0;
}
