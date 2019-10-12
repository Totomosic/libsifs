#include "sifsutils.h"
#include <string.h>
#include <stdio.h>

// Helper function that finds the fileblock that references datablockId as its data
SIFS_FILEBLOCK* find_fileblock(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID datablockId, SIFS_BLOCKID* outBlockId)
{
    // Iterate over all blocks (ignore root directory)
    for (SIFS_BLOCKID i = SIFS_ROOTDIR_BLOCKID + 1; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_FILE)
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, i);
            // Check if the fileblock references the datablockId
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
    // No fileblock references the datablock
    return NULL;
}

// Helper function that updates the entries of all directories that reference currentIndex
// Sets the entry to reference newIndex
void update_references(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID currentIndex, SIFS_BLOCKID newIndex)
{
    // Iterate through all blocks
    for (SIFS_BLOCKID i = 0; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_DIR && i != currentIndex)
        {
            SIFS_DIRBLOCK* dir = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, i);
            bool updated = false;
            // Find any entry that references currentIndex and set to newIndex
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
                // If we modified the directory, update its data back into the volume
                SIFS_updateblock(volumename, i, dir, 0);
            }
            free(dir);
        }
    }
}

// Helper function that moves a directory block from currentIndex to newIndex
void move_dirblock(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID currentIndex, SIFS_BLOCKID newIndex)
{
    SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, currentIndex);
    // Update all entries that refer to this directory
    update_references(volumename, header, bitmap, currentIndex, newIndex);
    // Free the existing directory block and update local copy of the bitmap
    SIFS_freeblocks(volumename, currentIndex, 1);
    bitmap[currentIndex] = SIFS_UNUSED;
    // Allocate a new block for the directory and update local copy of the bitmap
    SIFS_BLOCKID blockId = SIFS_allocateblocks(volumename, 1, SIFS_DIR);
    bitmap[blockId] = SIFS_DIR;
    if (blockId != newIndex || blockId == SIFS_ROOTDIR_BLOCKID)
    {
        // If we get here, something has gone horribly wrong (newIndex is not valid?)
        // In theory, should never get here
        return;
    }
    // Update the volume to reflect moved directory
    SIFS_updateblock(volumename, newIndex, dirblock, 0);
    free(dirblock);
}

// Helper function that moves a file block from currentIndex to newIndex
void move_fileblock(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, SIFS_BLOCKID currentIndex, SIFS_BLOCKID newIndex)
{
    SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, currentIndex);
    // Update all entries that refer to this file
    update_references(volumename, header, bitmap, currentIndex, newIndex);
    // Free the existing file block and update the local copy of the bitmap
    SIFS_freeblocks(volumename, currentIndex, 1);
    bitmap[currentIndex] = SIFS_UNUSED;
    // Alocate a new block for the file and update local copy of the bitmap
    SIFS_BLOCKID blockId = SIFS_allocateblocks(volumename, 1, SIFS_FILE);
    bitmap[blockId] = SIFS_FILE;
    if (blockId != newIndex || blockId == SIFS_ROOTDIR_BLOCKID)
    {
        // If we get here, something has gone horribly wrong (newIndex is not valid?)
        // In theory, should never get here
        return;
    }
    // Update the volume to reflect moved file
    SIFS_updateblock(volumename, newIndex, fileblock, 0);
    free(fileblock);
}

// Helper function that moves n datablocks that start at currentIndex to start at newIndex
void move_datablocks(const char* volumename, SIFS_VOLUME_HEADER* header, SIFS_BIT* bitmap, 
    SIFS_BLOCKID currentIndex, SIFS_BLOCKID nblocks, SIFS_BLOCKID newIndex, SIFS_FILEBLOCK* fileblock)
{
    // Get a pointer to the data
    void* dataPtr = SIFS_getblocks(volumename, currentIndex, nblocks);
    // Free the previous data blocks and update local copy of the bitmap
    SIFS_freeblocks(volumename, currentIndex, nblocks);
    for (SIFS_BLOCKID i = currentIndex; i < currentIndex + nblocks; i++)
    {
        bitmap[i] = SIFS_UNUSED;
    }
    // Allocate new datablocks and update local copy of the bitmap
    SIFS_BLOCKID blockId = SIFS_allocateblocks(volumename, nblocks, SIFS_DATABLOCK);
    for (SIFS_BLOCKID i = blockId; i < blockId + nblocks; i++)
    {
        bitmap[i] = SIFS_DATABLOCK;
    }
    if (blockId != newIndex || blockId == SIFS_ROOTDIR_BLOCKID)
    {
        // If we get here, something has gone horribly wrong (newIndex is not valid?)
        // In theory, should never get here
        return;
    }
    // Update the fileblock that references the data
    fileblock->firstblockID = newIndex;
    // Update the data in the volume, the fileblock will be written back to the volume elsewhere
    SIFS_updateblock(volumename, newIndex, dataPtr, fileblock->length);
    free(dataPtr);
}

int SIFS_defrag(const char *volumename)
{
    if (volumename == NULL)
    {
        SIFS_errno = SIFS_EINVAL;
        return SIFS_FAILURE;
    }

    SIFS_VOLUME_HEADER header;
    if (SIFS_getvolumeheader(volumename, &header) == SIFS_FAILURE)
    {
        // SIFS_errno set in SIFS_getvolumeheader()
        return SIFS_FAILURE;
    }
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    if (bitmap == NULL)
    {
        // SIFS_errno set in SIFS_getvolumebitmap()
        return SIFS_FAILURE;
    }

    SIFS_BLOCKID freeblockId = SIFS_ROOTDIR_BLOCKID;
    // Find the first freeblock available
    for (SIFS_BLOCKID i = SIFS_ROOTDIR_BLOCKID + 1; i < header.nblocks; i++)
    {
        if (bitmap[i] == SIFS_UNUSED && freeblockId == SIFS_ROOTDIR_BLOCKID)
        {
            freeblockId = i;
        }
        // Check if the current block is in use and there is a freeblock to the left of it
        else if (freeblockId != SIFS_ROOTDIR_BLOCKID && bitmap[i] != SIFS_UNUSED)
        {
            // Move these block(s) to the left to fill the space
            if (bitmap[i] == SIFS_FILE)
            {
                move_fileblock(volumename, &header, bitmap, i, freeblockId);
            }
            else if (bitmap[i] == SIFS_DIR)
            {
                move_dirblock(volumename, &header, bitmap, i, freeblockId);
            }
            else if (bitmap[i] == SIFS_DATABLOCK)
            {
                // Also need to find the fileblock that references this data and update it
                SIFS_BLOCKID fileblockId;
                SIFS_FILEBLOCK* fileblock = find_fileblock(volumename, &header, bitmap, i, &fileblockId);
                SIFS_BLOCKID nblocks = SIFS_calcnblocks(&header, fileblock->length);
                move_datablocks(volumename, &header, bitmap, i, nblocks, freeblockId, fileblock);
                SIFS_updateblock(volumename, fileblockId, fileblock, 0);
                free(fileblock);
            }
            // Modified the layout of the volume, go back and search for free blocks where we started
            i = freeblockId;
            freeblockId = SIFS_ROOTDIR_BLOCKID;
        }
    }

    free(bitmap);
    SIFS_errno = SIFS_EOK;
    return SIFS_SUCCESS;
}