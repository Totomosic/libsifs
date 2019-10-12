#include "sifsutils.h"
#include <string.h>
#include <stdio.h>

// remove an existing file from an existing volume
int SIFS_rmfile(const char *volumename, const char *pathname)
{
    if (volumename == NULL || pathname == NULL || strlen(pathname) == 0)
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
    // Check whether trying to delete root directory
    if (count == 0)
    {
        freesplit(result);
        SIFS_errno = SIFS_EINVAL;
        return SIFS_FAILURE;
    }

    char* filename = result[count - 1];
    // Find the directory that the file should be in
    SIFS_BLOCKID dirblockId;
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count - 1, &dirblockId);
    if (dir == NULL)
    {
        // Did not find a directory to remove the file from
        // SIFS_errno set in SIFS_getdir()
        freesplit(result);
        return SIFS_FAILURE;
    }
    
    // Check whether there is any entry with the filename (either directory or file)
    if (!SIFS_hasentry(volumename, dir, filename))
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_ENOENT;
        return SIFS_FAILURE;
    }
    SIFS_BLOCKID blockId = SIFS_ROOTDIR_BLOCKID;
    int entryId = -1;
    int fileIndex = -1;
    // Iterate over all directory entries to find a file entry with filename
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_FILE)
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            if (strcmp(fileblock->filenames[dir->entries[i].fileindex], filename) == 0)
            {
                // Found the file block entry
                // Record the directory's entryId, the BLOCKID of the file and the index of the filename
                entryId = i;
                blockId = dir->entries[i].blockID;
                fileIndex = dir->entries[i].fileindex;
                free(fileblock);
                break;
            }
            free(fileblock);
        }
    }
    // Failed to find a file entry, therefore the entry with the same name as filename must be a directory
    // Else we would've returned above
    if (entryId == -1)
    {
        freesplit(result);
        free(dir);
        SIFS_errno = SIFS_ENOTFILE;
        return SIFS_FAILURE;
    }
    SIFS_VOLUME_HEADER header;
    if (SIFS_getvolumeheader(volumename, &header) == SIFS_FAILURE)
    {
        // SIFS_errno set in SIFS_getvolumeheader()
        free(dir);
        freesplit(result);
        return SIFS_FAILURE;
    }
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    // Iterate through all directories in the volume and find those that reference this fileblock
    for (SIFS_BLOCKID i = 0; i < header.nblocks; i++)
    {
        if (bitmap[i] == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, i);
            for (int j = 0; j < dirblock->nentries; j++)
            {
                // Check if this directory entry references the file we are trying to remove
                // Only need to decrement the fileindex if its fileindex is greater than the index we are removing (ie. to the right of the file we are removing)
                if (i != dirblockId && dirblock->entries[j].blockID == blockId && dirblock->entries[j].fileindex > fileIndex)
                {
                    dirblock->entries[j].fileindex--;
                }
            }
            // Update the directory and write it back to the volume
            SIFS_updateblock(volumename, i, dirblock, 0);
            free(dirblock);
        }
    }
    // Perform the same operations as above on the directory that the file is being removed from
    // The same fileblock could be referenced multiple times within the same directory
    for (int j = 0; j < dir->nentries; j++)
    {
        if (dir->entries[j].blockID == blockId && dir->entries[j].fileindex > fileIndex)
        {
            dir->entries[j].fileindex--;
        }
    }
    // Get the fileblock that contains the file we are removing
    // Any filename that is right of the one being removed, shift left by 1
    SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, blockId);
    for (int i = fileIndex; i < fileblock->nfiles - 1; i++)
    {
        memcpy(fileblock->filenames[i], fileblock->filenames[i + 1], SIFS_MAX_NAME_LENGTH);
    }
    // Clear the last filenamename
    memset(fileblock->filenames[fileblock->nfiles - 1], 0, SIFS_MAX_NAME_LENGTH);
    fileblock->nfiles--;

    // Check whether this was the last file that this fileblock referenced
    if (fileblock->nfiles <= 0)
    {
        // Free the data blocks and the fileblock
        SIFS_BLOCKID nblocks = SIFS_calcnblocks(&header, fileblock->length);
        SIFS_freeblocks(volumename, fileblock->firstblockID, nblocks);
        SIFS_freeblocks(volumename, blockId, 1);
    }
    // Rewrite the fileblock back to the volume
    SIFS_updateblock(volumename, blockId, fileblock, 0);
    free(fileblock);
    // Any entry in the directory that the file is being removed from that is to the right needs to be shifted left by 1
    for (int i = entryId; i < dir->nentries - 1; i++)
    {
        dir->entries[i] = dir->entries[i + 1];
    }
    // Update directory metadata
    dir->nentries--;
    dir->modtime = time(NULL);
    // Rewrite directory back to volume
    SIFS_updateblock(volumename, dirblockId, dir, 0);

    freesplit(result);
    free(dir);
    free(bitmap);
    SIFS_errno = SIFS_EOK;
    return SIFS_SUCCESS;
}
