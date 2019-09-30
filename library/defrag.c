#include "sifsutils.h"
#include <string.h>
#include <stdio.h>

SIFS_FILEBLOCK* find_fileblock(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID datablockId, SIFS_BLOCKID* outBlockId)
{
    for (SIFS_BLOCKID i = SIFS_ROOTDIR_BLOCKID + 1; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_FILE)
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, i);
            if (fileblock->firstblockID == datablockId)
            {
                if (outBlockId)
                {
                    *outBlockId = i;
                }
                return fileblock;
            }
            free(fileblock);
        }
    }
    return NULL;
}

void update_references(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID currentIndex, SIFS_BLOCKID newIndex)
{
    for (SIFS_BLOCKID i = 0; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_DIR && i != currentIndex)
        {
            SIFS_DIRBLOCK* dir = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, i);
            bool updated = false;
            for (int i = 0; i < dir->nentries; i++)
            {
                if (dir->entries[i].blockID == currentIndex)
                {
                    dir->entries[i].blockID = newIndex;
                    updated = true;
                }
            }
            if (updated)
            {
                SIFS_updateblock(volumename, i, dir, 0);
            }
            free(dir);
        }
    }
}

void move_dirblock(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID currentIndex, SIFS_BLOCKID newIndex)
{
    SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, currentIndex);
    // Update all entries that refer to this directory
    update_references(volumename, header, bitmap, currentIndex, newIndex);
    SIFS_freeblocks(volumename, currentIndex, 1);
    SIFS_BLOCKID blockId = SIFS_allocateblocks(volumename, 1, SIFS_DIR);
    if (blockId != newIndex)
    {
        printf("Failed, expected %i actual %i\n", newIndex, blockId);
        return;
    }
    SIFS_updateblock(volumename, newIndex, dirblock, 0);
}

void move_fileblock(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID currentIndex, SIFS_BLOCKID newIndex)
{
    SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, currentIndex);
    // Update all entries that refer to this directory
    update_references(volumename, header, bitmap, currentIndex, newIndex);
    SIFS_freeblocks(volumename, currentIndex, 1);
    SIFS_BLOCKID blockId = SIFS_allocateblocks(volumename, 1, SIFS_FILE);
    if (blockId != newIndex)
    {
        printf("Failed, expected %i actual %i\n", newIndex, blockId);
        return;
    }
    SIFS_updateblock(volumename, newIndex, fileblock, 0);
}

void move_datablock(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, 
    SIFS_BLOCKID currentIndex, SIFS_BLOCKID nblocks, SIFS_BLOCKID newIndex, SIFS_FILEBLOCK* fileblock)
{
    void* dataPtr = SIFS_getblocks(volumename, currentIndex, nblocks);
    SIFS_freeblocks(volumename, currentIndex, nblocks);
    SIFS_BLOCKID blockId = SIFS_allocateblocks(volumename, nblocks, SIFS_DATABLOCK);
    if (blockId != newIndex)
    {
        printf("Failed, expected %i actual %i\n", newIndex, blockId);
        return;
    }
    fileblock->firstblockID = newIndex;
    SIFS_updateblock(volumename, newIndex, dataPtr, fileblock->length);
    free(dataPtr);
}

int SIFS_defrag(const char *volumename)
{
    if (volumename == NULL)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    if (header == NULL || bitmap == NULL)
    {
        return 1;
    }

    SIFS_BLOCKID freeblockId = SIFS_ROOTDIR_BLOCKID;
    for (SIFS_BLOCKID i = SIFS_ROOTDIR_BLOCKID + 1; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_UNUSED && freeblockId == SIFS_ROOTDIR_BLOCKID)
        {
            freeblockId = i;
        }
        else if (freeblockId != SIFS_ROOTDIR_BLOCKID && bitmap[i] != SIFS_UNUSED)
        {
            if (bitmap[i] == SIFS_FILE)
            {
                move_fileblock(volumename, header, bitmap, i, freeblockId);
            }
            else if (bitmap[i] == SIFS_DIR)
            {
                move_dirblock(volumename, header, bitmap, i, freeblockId);
            }
            else if (bitmap[i] == SIFS_DATABLOCK)
            {
                SIFS_BLOCKID fileblockId;
                SIFS_FILEBLOCK* fileblock = find_fileblock(volumename, header, bitmap, i, &fileblockId);
                SIFS_BLOCKID nblocks = SIFS_calcnblocks(header, fileblock->length);
                move_datablock(volumename, header, bitmap, i, nblocks, freeblockId, fileblock);
                SIFS_updateblock(volumename, fileblockId, fileblock, 0);
                free(fileblock);
            }
            i = freeblockId;
            free(bitmap);
            bitmap = SIFS_getvolumebitmap(volumename);
            freeblockId = SIFS_ROOTDIR_BLOCKID;
        }
    }

    free(bitmap);
    free(header);
    SIFS_errno = SIFS_EOK;
    return 0;
}