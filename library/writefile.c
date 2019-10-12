#include "sifsutils.h"
#include <string.h>
#include <stdio.h>

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{
    if (volumename == NULL || pathname == NULL || data == NULL || nbytes == 0)
    {
        SIFS_errno = SIFS_EINVAL;
        return SIFS_ERROR;
    }

    size_t count;
    char** result = strsplit(pathname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return SIFS_ERROR;
    }
    char* filename = result[count - 1];
    size_t filenameLength = strlen(filename);
    if (filenameLength == 0 || filenameLength >= SIFS_MAX_NAME_LENGTH)
    {
        freesplit(result);
        SIFS_errno = SIFS_EINVAL;
        return SIFS_ERROR;
    }
    // Find directory to place file in
    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count - 1, &dirblockId);
    if (dir == NULL)
    {
        freesplit(result);
        return SIFS_ERROR;
    }
    if (dir->nentries >= SIFS_MAX_ENTRIES)
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_EMAXENTRY;
        return SIFS_ERROR;
    }
    if (SIFS_hasentry(volumename, dir, filename))
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_EEXIST;
        return SIFS_ERROR;
    }
    
    unsigned char md5[MD5_BYTELEN];
    MD5_buffer(data, nbytes, md5);
    SIFS_BLOCKID blockId;
    SIFS_FILEBLOCK* block = SIFS_getfileblock(volumename, md5, &blockId);
    if (block != NULL)
    {
        if (block->nfiles >= SIFS_MAX_ENTRIES)
        {
            freesplit(result);
            free(dir);
            free(block);
            SIFS_errno = SIFS_EMAXENTRY;
            return SIFS_ERROR;
        }
    }
    else
    {
        SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
        SIFS_BLOCKID nblocks = SIFS_calcnblocks(&header, nbytes);
        SIFS_BLOCKID fileblockId = SIFS_allocateblocks(volumename, 1, SIFS_FILE);
        SIFS_BLOCKID datablockId = SIFS_allocateblocks(volumename, nblocks, SIFS_DATABLOCK);
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
            return SIFS_ERROR;
        }
        void* dataPtr = SIFS_getblocks(volumename, datablockId, nblocks);
        memcpy(dataPtr, data, nbytes);
        SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, fileblockId);
        fileblock->modtime = time(NULL);
        memcpy(fileblock->md5, md5, MD5_BYTELEN);
        fileblock->length = nbytes;
        fileblock->firstblockID = datablockId;
        fileblock->nfiles = 0;
        block = fileblock;
        blockId = fileblockId;
        SIFS_updateblock(volumename, datablockId, dataPtr, nbytes);
        free(dataPtr);
    }
    memset(block->filenames[block->nfiles], 0, SIFS_MAX_NAME_LENGTH);
    memcpy(block->filenames[block->nfiles++], filename, filenameLength);
    dir->entries[dir->nentries].blockID = blockId;
    dir->entries[dir->nentries++].fileindex = block->nfiles - 1;
    dir->modtime = time(NULL);

    SIFS_updateblock(volumename, dirblockId, dir, 0);
    SIFS_updateblock(volumename, blockId, block, 0);

    freesplit(result);
    free(dir);
    free(block);
    SIFS_errno = SIFS_EOK;
    return SIFS_OK;
}
