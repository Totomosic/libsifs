#include "sifsutils.h"
#include <string.h>

// add a copy of a new file to an existing volume
int SIFS_writefile(const char *volumename, const char *pathname,
		   void *data, size_t nbytes)
{
    if (volumename == NULL || pathname == NULL || data == NULL || nbytes == 0)
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
        return 1;
    }
    char* filename = result[count - 1];
    size_t filenameLength = strlen(filename);
    if (filenameLength == 0 || filenameLength >= SIFS_MAX_NAME_LENGTH)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    // Find directory to place file in
    SIFS_DIRBLOCK* dir = SIFS_getdir(volume, result, count - 1);
    if (dir == NULL)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_ENOTDIR;
        return 1;
    }
    if (dir->nentries >= SIFS_MAX_ENTRIES)
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_EMAXENTRY;
        return 1;
    }
    if (SIFS_hasentry(volume, dir, filename))
    {
        freesplit(result);
        free(volume);
        SIFS_errno = SIFS_EEXIST;
        return 1;
    }

    unsigned char md5[MD5_BYTELEN];
    MD5_buffer(data, nbytes, md5);
    SIFS_BLOCKID blockId;
    SIFS_FILEBLOCK* block = SIFS_getfileblock(volume, md5, &blockId);
    if (block != NULL)
    {
        if (block->nfiles >= SIFS_MAX_ENTRIES)
        {
            freesplit(result);
            free(volume);
            SIFS_errno = SIFS_EMAXENTRY;
            return 1;
        }
    }
    else
    {
        SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volume);
        int nblocks = nbytes / header->blocksize + ((nbytes % header->blocksize == 0) ? 0 : 1);
        SIFS_BLOCKID fileblockId = SIFS_allocateblocks(volume, 1, SIFS_FILE);
        SIFS_BLOCKID datablockId = SIFS_allocateblocks(volume, nblocks, SIFS_DATABLOCK);
        if (fileblockId == SIFS_ROOTDIR_BLOCKID || datablockId == SIFS_ROOTDIR_BLOCKID)
        {
            freesplit(result);
            free(volume);
            SIFS_errno = SIFS_ENOSPC;
            if (fileblockId != SIFS_ROOTDIR_BLOCKID)
            {
                SIFS_freeblocks(volume, fileblockId, 1);
            }
            if (datablockId != SIFS_ROOTDIR_BLOCKID)
            {
                SIFS_freeblocks(volume, datablockId, nblocks);
            }
            return 1;
        }
        void* dataPtr = SIFS_getblock(volume, datablockId);
        memcpy(dataPtr, data, nbytes);
        SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volume, fileblockId);
        fileblock->modtime = time(NULL);
        memcpy(fileblock->md5, md5, MD5_BYTELEN);
        fileblock->length = nbytes;
        fileblock->firstblockID = datablockId;
        fileblock->nfiles = 0;
        block = fileblock;
        blockId = fileblockId;
    }
    memcpy(block->filenames[block->nfiles++], filename, filenameLength);
    dir->entries[dir->nentries].blockID = blockId;
    dir->entries[dir->nentries++].fileindex = block->nfiles - 1;

    freesplit(result);
    SIFS_rewritevolume(volumename, volume);
    free(volume);
    SIFS_errno = SIFS_EOK;
    return 0;
}
