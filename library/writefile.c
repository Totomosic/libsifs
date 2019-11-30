#include "sifsutils.h"
#include <string.h>
#include <stdio.h>

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{
    if (volumename == NULL || pathname == NULL || data == NULL || strlen(pathname) == 0)
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
        SIFS_errno = SIFS_EINVAL;
        freesplit(result);
        return SIFS_FAILURE;
    }

    char* filename = result[count - 1];
    size_t filenameLength = strlen(filename);
    if (filenameLength == 0 || filenameLength >= SIFS_MAX_NAME_LENGTH)
    {
        freesplit(result);
        SIFS_errno = SIFS_EINVAL;
        return SIFS_FAILURE;
    }
    // Find directory to place file in
    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count - 1, &dirblockId);
    if (dir == NULL)
    {
        freesplit(result);
        return SIFS_FAILURE;
    }
    // Check if dir has enough entries to add a new file
    if (dir->nentries >= SIFS_MAX_ENTRIES)
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_EMAXENTRY;
        return SIFS_FAILURE;
    }
    // Check if the dir already has an entry with the same name (directory or file)
    if (SIFS_hasentry(volumename, dir, filename))
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_EEXIST;
        return SIFS_FAILURE;
    }
    
    // Calculate the md5 for the given data
    unsigned char md5[MD5_BYTELEN];
    MD5_buffer(data, nbytes, md5);
    // Try to find a file block with the same md5 (only storing the contents of file once)
    SIFS_BLOCKID blockId;
    SIFS_FILEBLOCK* block = SIFS_getfileblock(volumename, md5, &blockId);
    if (block != NULL)
    {
        // Found a fileblock with the same md5, ensure that it has enough remaining filenames to create a new one
        if (block->nfiles >= SIFS_MAX_ENTRIES)
        {
            freesplit(result);
            free(dir);
            free(block);
            SIFS_errno = SIFS_EMAXENTRY;
            return SIFS_FAILURE;
        }
    }
    else
    {
        // No fileblock with the same md5, create a new one
        SIFS_VOLUME_HEADER header;
        if (SIFS_getvolumeheader(volumename, &header) == SIFS_FAILURE)
        {
            // SIFS_errno set in SIFS_getvolumeheader()
            freesplit(result);
            free(dir);
            free(block);
            return SIFS_FAILURE;
        }
        SIFS_BLOCKID nblocks = SIFS_calcnblocks(&header, nbytes);
        // Allocate a block to store the file metadata as well as enough blocks to fit the file's data
        SIFS_BLOCKID fileblockId = SIFS_allocateblocks(volumename, 1, SIFS_FILE);
        SIFS_BLOCKID datablockId = SIFS_allocateblocks(volumename, nblocks, SIFS_DATABLOCK);
        // Check whether either allocation failed
        if (fileblockId == SIFS_ROOTDIR_BLOCKID || datablockId == SIFS_ROOTDIR_BLOCKID)
        {
            freesplit(result);
            free(dir);
            SIFS_errno = SIFS_ENOSPC;
            if (fileblockId != SIFS_ROOTDIR_BLOCKID)
            {
                SIFS_freeblocks(volumename, fileblockId, 1);
            }
            if (datablockId != SIFS_ROOTDIR_BLOCKID)
            {
                SIFS_freeblocks(volumename, datablockId, nblocks);
            }
            return SIFS_FAILURE;
        }
        // Setup fileblock metadata
        SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, fileblockId);
        fileblock->modtime = time(NULL);
        memcpy(fileblock->md5, md5, MD5_BYTELEN);
        fileblock->length = nbytes;
        fileblock->firstblockID = datablockId;
        fileblock->nfiles = 0;
        block = fileblock;
        blockId = fileblockId;
        if (nblocks > 0)
        {
            // Get a pointer to the data to write to and copy data into them
            void* dataPtr = SIFS_getblocks(volumename, datablockId, nblocks);
            memcpy(dataPtr, data, nbytes);
            // Write the datablocks back into the volume
            SIFS_updateblock(volumename, datablockId, dataPtr, nbytes);
            free(dataPtr);
        }
    }
    // Set the filename that we are about to write to to 0s (could be in the same place a data from a previous file and therefore not null terminated correctly)
    // Copy filename into correct spot
    memset(block->filenames[block->nfiles], 0, SIFS_MAX_NAME_LENGTH);
    memcpy(block->filenames[block->nfiles++], filename, filenameLength);
    // Update the entries in the parent directory
    dir->entries[dir->nentries].blockID = blockId;
    dir->entries[dir->nentries++].fileindex = block->nfiles - 1;
    dir->modtime = time(NULL);

    // Rewrite the parent directory to the volume
    SIFS_updateblock(volumename, dirblockId, dir, 0);
    // Rewrite the fileblock to the volume
    SIFS_updateblock(volumename, blockId, block, 0);

    freesplit(result);
    free(dir);
    free(block);
    SIFS_errno = SIFS_EOK;
    return SIFS_SUCCESS;
}
