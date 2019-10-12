#include "sifsutils.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>

char** strsplit(const char* str, char delimiter, size_t* outCount)
{
    // Calculate and allocate enough memory to copy the string
    size_t strlength = strlen(str);
    char* buffer = malloc(sizeof(char) * (strlength + 1));
    if (buffer == NULL)
    {
        return NULL;
    }
    strcpy(buffer, str);
    size_t count = 0;

    // Count the number of potential elements in the resulting vector (count number of delimiters that aren't consecutive)
    char prev = delimiter;
    for (int i = 0; i < strlength; i++)
    {
        if (buffer[i] == delimiter)
        {
            // Set all delimiter characters to the null byte
            buffer[i] = '\0';
            if (prev != delimiter)
            {
                count++;
            }
        }
        prev = str[i];
    }
    // Always need to store on more string that the number of delimiters unless the last character is a delimiter or str is the empty string
    if (strlength > 0 && str[strlength - 1] != delimiter)
    {
        count++;
    }

    // Allocate memory to store vector of strings
    char** vector = (char**)malloc(sizeof(char*) * count);
    if (vector == NULL)
    {
        return NULL;
    }

    // Iterate through elements of string and set all delimiter characters to '\0'
    // If the string begins with several delimiter characters, erase them by keeping track of the first non delimiter character (nonNull)
    prev = '\0';
    int index = 0;
    int nonNull = -1;
    for (int i = 0; i < strlength; i++)
    {
        // If the current character is not '\0' but the previous character was,
        // This must be the start of a new string, add its pointer to the vector
        if (buffer[i] != '\0' && prev == '\0')
        {
            // Find the first non delimiter character
            if (nonNull == -1)
            {
                nonNull = i;
            }
            // - nonNull to account for any shifting that will occur later due to string beginning with delimiter characters
            vector[index++] = buffer + i - nonNull;
        }
        prev = buffer[i];
    }

    // The string began with delimiter characters, shift everything left to erase them
    if (nonNull != -1 && nonNull != 0) 
    {
        memmove(buffer, buffer + nonNull, strlength - nonNull);
    }

    if (outCount != NULL)
    {
        *outCount = count;
    }
    return vector;
}

void freesplit(char** strsplitresult)
{
    // Ptr to the allocated buffer will always be the first element
    free(strsplitresult[0]);
    free(strsplitresult);
}

int SIFS_readvolumeptr(const char* volumename, void* data, size_t offset, size_t length)
{
    // Read the size of the volume
    struct stat fStat;
    if (stat(volumename, &fStat) != 0)
    {
        SIFS_errno = SIFS_ENOVOL;
        return SIFS_FAILURE;
    }
    FILE* file = fopen(volumename, "rb");
    if (file == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return SIFS_FAILURE;
    }
    // Read the header of the volume (offset 0)
    SIFS_VOLUME_HEADER header;
    fread(&header, sizeof(SIFS_VOLUME_HEADER), 1, file);
    // Read data from the volume at offset
    fseek(file, offset, SEEK_SET);
    fread(data, 1, length, file);
    
    // Validate whether this file represents a volume by checking that its size is what the header says it should be
    // A file that does not represent a volume could potentially pass this test, very unlikely
    size_t expectedLength = header.blocksize * header.nblocks + sizeof(SIFS_VOLUME_HEADER) + sizeof(SIFS_BIT) * header.nblocks;
    if (fStat.st_size != expectedLength)
    {
        SIFS_errno = SIFS_ENOTVOL;
        fclose(file);
        return SIFS_FAILURE;
    }
    fclose(file);
    return SIFS_SUCCESS;
}

// Helper function for reading a volume easier
void* SIFS_readvolume(const char* volumename, size_t offset, size_t length)
{
    void* data = malloc(length);
    if (data == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return NULL;
    }
    int result = SIFS_readvolumeptr(volumename, data, offset, length);
    if (result == SIFS_FAILURE) 
    {
        free(data);
        return NULL;
    }
    return data;
}

int SIFS_updatevolume(const char* volumename, size_t offset, const void* data, size_t nbytes)
{
    // Open volume for reading and writing so that we can modify the existing contents of the volume
    FILE* file = fopen(volumename, "rb+");
    if (file == NULL)
    {
        return SIFS_FAILURE;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(data, 1, nbytes, file);
    fclose(file);
    return SIFS_SUCCESS;
}

SIFS_VOLUME_HEADER SIFS_getvolumeheader(const char* volumename)
{
    SIFS_VOLUME_HEADER header;
    SIFS_readvolumeptr(volumename, &header, 0, sizeof(SIFS_VOLUME_HEADER));
    return header;
}

SIFS_BIT* SIFS_getvolumebitmap(const char* volumename)
{
    SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
    SIFS_BIT* ptr = (SIFS_BIT*)SIFS_readvolume(volumename, sizeof(SIFS_VOLUME_HEADER), header.nblocks * sizeof(SIFS_BIT));
    return ptr;
}

SIFS_BLOCKID SIFS_calcnblocks(SIFS_VOLUME_HEADER* header, size_t nbytes)
{
    SIFS_BLOCKID nblocks = nbytes / header->blocksize + ((nbytes % header->blocksize == 0) ? 0 : 1);
    return nblocks;
}

void SIFS_updatevolumebitmap(const char* volumename, const SIFS_BIT* bitmap, size_t length)
{
    if (length == 0)
    {
        SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
        length = header.nblocks;
    }
    SIFS_updatevolume(volumename, sizeof(SIFS_VOLUME_HEADER), (void*)bitmap, length);
}

void SIFS_updateblock(const char* volumename, SIFS_BLOCKID blockIndex, const void* data, size_t length)
{
    SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
    if (length == 0)
    {
        length = header.blocksize;
    }
    size_t offset = sizeof(SIFS_VOLUME_HEADER) + header.nblocks * sizeof(SIFS_BIT) + blockIndex * header.blocksize;
    SIFS_updatevolume(volumename, offset, data, length);
}

void* SIFS_getblock(const char* volumename, SIFS_BLOCKID blockIndex)
{
    return SIFS_getblocks(volumename, blockIndex, 1);
}

void* SIFS_getblocks(const char* volumename, SIFS_BLOCKID first, SIFS_BLOCKID nblocks)
{
    SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
    size_t offset = sizeof(SIFS_VOLUME_HEADER) + sizeof(SIFS_BIT) * header.nblocks + header.blocksize * first;
    void* ptr = SIFS_readvolume(volumename, offset, header.blocksize * nblocks);
    return ptr;
}

SIFS_DIRBLOCK* SIFS_getrootdir(const char* volumename)
{
    return (SIFS_DIRBLOCK*)SIFS_getblock(volumename, SIFS_ROOTDIR_BLOCKID);
}

SIFS_DIRBLOCK* SIFS_getdir(const char* volumename, char** dirnames, size_t dircount, SIFS_BLOCKID* outBlockId)
{
    SIFS_DIRBLOCK* root = SIFS_getrootdir(volumename);
    if (root == NULL)
    {
        return NULL;
    }
    // Use "." as an alias for the root directory
    if (dircount > 0 && (strcmp(dirnames[0], ".") == 0))
    {
        dircount -= 1;
        dirnames += 1;
    }
    // Reprensents the root directory
    if (dircount == 0)
    {
        if (outBlockId != NULL)
        {
            *outBlockId = SIFS_ROOTDIR_BLOCKID;
        }
        return root;
    }
    // Recursively search through path to find directory
    return SIFS_finddir(volumename, root, dirnames, dircount, outBlockId);
}

SIFS_DIRBLOCK* SIFS_finddir(const char* volumename, SIFS_DIRBLOCK* dir, char** dirnames, size_t dircount, SIFS_BLOCKID* outBlockId)
{
    // Search through the current directory's entries and find one that matches the first directory name (dirnames[0])
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* block = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            if (strcmp(block->name, dirnames[0]) == 0)
            {
                // Found correct directory
                // If this directory was the last in dirnames (dircount == 1), return this directory block
                // Else find the next directory using the next dirname in dirnames
                if (dircount == 1)
                {
                    if (outBlockId != NULL)
                    {
                        *outBlockId = dir->entries[i].blockID;
                    }
                    // Free the parent block
                    free(dir);
                    return block;
                }
                return SIFS_finddir(volumename, block, dirnames + 1, dircount - 1, outBlockId);
            }
            free(block);
        }
    }
    // Failed to find a directory entry with the correct name
    SIFS_errno = SIFS_ENOENT;
    free(dir);
    return NULL;
}

SIFS_FILEBLOCK* SIFS_getfile(const char* volumename, char** path, size_t count, SIFS_BLOCKID* outFileIndex)
{
    if (count == 0)
    {
        SIFS_errno = SIFS_ENOTFILE;
        return NULL;
    }
    // Last element in path is the filename, everything before that represents the directory
    // Find the directory from the path
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, path, count - 1, NULL);
    if (dir == NULL)
    {
        // SIFS_errno set in SIFS_getdir()
        return NULL;
    }
    char* filename = path[count - 1];
    // Try to find the file in the directory
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_FILE)
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            if (strcmp(fileblock->filenames[dir->entries[i].fileindex], filename) == 0)
            {
                if (outFileIndex != NULL)
                {
                    *outFileIndex = dir->entries[i].fileindex;
                }
                free(dir);
                return fileblock;
            }
            free(fileblock);
        }
    }
    // Unable to a find a file with the correct name
    // Test whether the directory has any entry with the correct name (eg. a directory with the filename)
    if (SIFS_hasentry(volumename, dir, filename))
    {
        // The directory contains a directory with the filename
        free(dir);
        SIFS_errno = SIFS_ENOTFILE;
        return NULL;
    }
    // No entry named filename
    free(dir);
    SIFS_errno = SIFS_ENOENT;
    return NULL;
}

SIFS_BIT SIFS_getblocktype(const char* volumename, SIFS_BLOCKID blockIndex)
{
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    if (bitmap == NULL)
    {
        return SIFS_UNUSED;
    }
    SIFS_BIT type = bitmap[blockIndex];
    free(bitmap);
    return type;
}

SIFS_BLOCKID SIFS_allocateblocks(const char* volumename, SIFS_BLOCKID nblocks, SIFS_BIT type)
{
    SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
    if (nblocks > header.nblocks)
    {
        // Tried to allocate too many blocks
        return SIFS_ROOTDIR_BLOCKID;
    }
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    // Try to find nblocks of contiguous blocks in the bitmap
    for (size_t i = SIFS_ROOTDIR_BLOCKID + 1; i < header.nblocks; i++)
    {
        if (bitmap[i] == SIFS_UNUSED)
        {
            bool available = true;
            for (SIFS_BLOCKID j = 1; j < nblocks; j++)
            {   
                if (i + j >= header.nblocks)
                {
                    // Tried to read past end of bitmap
                    available = false;
                    break;
                }
                if (bitmap[i + j] != SIFS_UNUSED)
                {
                    // Not a contigous section of blocks
                    available = false;
                    break;
                }
            }
            if (available)
            {
                // Found available blocks successfully
                // Update the volume bitmap to reflect this allocation
                // Return the BLOCKID of the first block allocated
                for (SIFS_BLOCKID j = 0; j < nblocks; j++)
                {   
                    bitmap[i + j] = type;
                }
                SIFS_updatevolumebitmap(volumename, bitmap, header.nblocks);
                free(bitmap);
                return i;
            }
        }
    }
    free(bitmap);
    return SIFS_ROOTDIR_BLOCKID;
}

void SIFS_freeblocks(const char* volumename, SIFS_BLOCKID firstblock, SIFS_BLOCKID nblocks)
{
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    if (bitmap == NULL)
    {
        return;
    }
    // Updates the bitmap to reflect deallocation
    for (SIFS_BLOCKID i = firstblock; i < firstblock + nblocks; i++)
    {
        bitmap[i] = SIFS_UNUSED;
    }
    SIFS_updatevolumebitmap(volumename, bitmap, 0);
    free(bitmap);
}

bool SIFS_hasentry(const char* volumename, SIFS_DIRBLOCK* directory, const char* entryname)
{
    // Tests whether directory has any entry named entryname (file or directory)
    for (int i = 0; i < directory->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, directory->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dir = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, directory->entries[i].blockID);
            if (strcmp(dir->name, entryname) == 0)
            {
                free(dir);
                return true;
            }
            free(dir);
        }
        else if (type == SIFS_FILE)
        {
            SIFS_FILEBLOCK* file = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, directory->entries[i].blockID);
            if (strcmp(file->filenames[directory->entries[i].fileindex], entryname) == 0)
            {
                free(file);
                return true;
            }
            free(file);
        }
    }
    return false;
}

SIFS_FILEBLOCK* SIFS_getfileblock(const char* volumename, const void* md5, SIFS_BLOCKID* outBlockid)
{
    SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    if (bitmap == NULL)
    {
        // SIFS_errno set in SIFS_getvolumebitmap()
        return NULL;
    }
    // Go through all blocks in the volume and find all file blocks
    for (SIFS_BLOCKID i = 0; i < header.nblocks; i++)
    {
        if (bitmap[i] == SIFS_FILE)
        {
            SIFS_FILEBLOCK* block = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, i);
            // Check if the md5 matches
            if (memcmp(md5, block->md5, MD5_BYTELEN) == 0)
            {
                if (outBlockid != NULL)
                {
                    *outBlockid = i;
                }
                free(bitmap);
                return block;
            }
            free(block);
        }
    }
    free(bitmap);
    return NULL;
}