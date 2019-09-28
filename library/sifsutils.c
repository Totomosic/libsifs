#include "sifsutils.h"
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdbool.h>

char** strsplit(const char* str, char delimeter, size_t* outCount)
{
    size_t strlength = strlen(str);
    char* buffer = malloc(sizeof(char) * (strlength + 1));
    if (buffer == NULL)
    {
        return NULL;
    }
    char* ptr = buffer;
    strcpy(ptr, str);
    size_t count = 0;

    char prev = delimeter;
    for (int i = 0; i < strlength; i++)
    {
        if (ptr[i] == delimeter)
        {
            ptr[i] = '\0';
            if (prev != delimeter)
            {
                count++;
            }
        }
        prev = str[i];
    }
    if (strlength > 0 && str[strlength - 1] != delimeter)
    {
        count++;
    }

    char** vector = (char**)malloc(sizeof(char*) * (count + 2));
    if (vector == NULL)
    {
        return NULL;
    }
    vector[0] = buffer;

    prev = '\0';
    int index = 1;
    for (int i = 0; i < strlength; i++)
    {
        if (ptr[i] != '\0' && prev == '\0')
        {
            vector[index++] = ptr + i;
        }
        prev = ptr[i];
    }

    vector[index] = NULL;
    if (outCount != NULL)
    {
        *outCount = count;
    }
    return vector + 1;
}

void freesplit(char** strsplitresult)
{
    free((strsplitresult - 1)[0]);
    free(strsplitresult - 1);
}

void* SIFS_readvolume(const char* volumename, size_t* outSize)
{
    struct stat fStat;
    if (stat(volumename, &fStat) != 0)
    {
        SIFS_errno = SIFS_ENOVOL;
        return NULL;
    }
    FILE* file = fopen(volumename, "r");
    if (file == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return NULL;
    }
    size_t size = fStat.st_size;
    void* data = malloc(size);
    if (data == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return NULL;
    }
    fread(data, 1, size, file);
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(data);
    if (size != header->blocksize * header->nblocks + sizeof(SIFS_VOLUME_HEADER) + sizeof(SIFS_BIT) * header->nblocks)
    {
        SIFS_errno = SIFS_ENOTVOL;
        free(data);
        fclose(file);
        return NULL;
    }
    if (outSize != NULL)
    {
        *outSize = size;
    }
    fclose(file);
    return data;
}

int SIFS_updatevolume(const char* volumename, size_t offset, const void* data, size_t nbytes)
{
    size_t fileLength;
    void* fileData = SIFS_readvolume(volumename, &fileLength);
    if (fileData == NULL)
    {
        return 1;
    }
    memcpy((char*)fileData + offset, data, nbytes);
    FILE* file = fopen(volumename, "w");
    if (file == NULL)
    {
        return 1;
    }
    fwrite(fileData, 1, fileLength, file);
    free(fileData);
    fclose(file);
    return 0;
}

int SIFS_rewritevolume(const char* volumename, void* volumedata)
{
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumedata);
    size_t length = sizeof(SIFS_VOLUME_HEADER) + sizeof(SIFS_BIT) * header->nblocks + header->nblocks * header->blocksize;
    return SIFS_updatevolume(volumename, 0, volumedata, length);
}

SIFS_VOLUME_HEADER* SIFS_getvolumeheader(void* volume)
{
    return (SIFS_VOLUME_HEADER*)volume;
}

SIFS_BIT* SIFS_getvolumebitmap(void* volume)
{
    return (SIFS_BIT*)((char*)volume + sizeof(SIFS_VOLUME_HEADER));
}

void* SIFS_getblock(void* volume, SIFS_BLOCKID blockIndex)
{
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volume);
    char* bytePtr = (char*)volume;
    size_t offset = sizeof(SIFS_VOLUME_HEADER) + sizeof(SIFS_BIT) * header->nblocks + header->blocksize * blockIndex;
    return (void*)(bytePtr + offset);
}

SIFS_DIRBLOCK* SIFS_getrootdir(void* volume)
{
    return (SIFS_DIRBLOCK*)SIFS_getblock(volume, SIFS_ROOTDIR_BLOCKID);
}

SIFS_DIRBLOCK* SIFS_getdir(void* volume, char** dirnames, size_t dircount)
{
    SIFS_DIRBLOCK* root = SIFS_getrootdir(volume);
    if (dircount > 0 && ((strcmp(dirnames[0], ".") == 0) || (strcmp(dirnames[0], "~") == 0)))
    {
        dircount -= 1;
        dirnames += 1;
    }
    if (dircount == 0)
    {
        return root;
    }
    return SIFS_finddir(volume, root, dirnames, dircount);
}

SIFS_DIRBLOCK* SIFS_finddir(void* volume, SIFS_DIRBLOCK* dir, char** dirnames, size_t dircount)
{
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volume, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* block = (SIFS_DIRBLOCK*)SIFS_getblock(volume, dir->entries[i].blockID);
            if (strcmp(block->name, dirnames[0]) == 0)
            {
                if (dircount == 1)
                {
                    return block;
                }
                return SIFS_finddir(volume, block, dirnames + 1, dircount - 1);
            }
        }
    }
    return NULL;
}

SIFS_FILEBLOCK* SIFS_getfile(void* volume, char** path, size_t count, SIFS_BLOCKID* outFileIndex)
{
    SIFS_DIRBLOCK* dir = SIFS_getdir(volume, path, count - 1);
    if (dir == NULL)
    {
        SIFS_errno = SIFS_ENOENT;
        return NULL;
    }
    char* filename = path[count - 1];
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volume, dir->entries[i].blockID);
        if (type == SIFS_FILE)
        {
            SIFS_FILEBLOCK* fileblock = (SIFS_FILEBLOCK*)SIFS_getblock(volume, dir->entries[i].blockID);
            if (strcmp(fileblock->filenames[dir->entries[i].fileindex], filename) == 0)
            {
                if (outFileIndex != NULL)
                {
                    *outFileIndex = dir->entries[i].fileindex;
                }
                return fileblock;
            }
        }
    }
    if (SIFS_hasentry(volume, dir, filename))
    {
        SIFS_errno = SIFS_ENOTFILE;
        return NULL;
    }
    SIFS_errno = SIFS_ENOENT;
    return NULL;
}

SIFS_BIT SIFS_getblocktype(void* volume, SIFS_BLOCKID blockIndex)
{
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volume);
    return bitmap[blockIndex];
}

SIFS_BLOCKID SIFS_allocateblocks(void* volume, SIFS_BLOCKID nblocks, SIFS_BIT type)
{
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volume);
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volume);
    for (size_t i = SIFS_ROOTDIR_BLOCKID + 1; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_UNUSED)
        {
            bool available = true;
            for (SIFS_BLOCKID j = 1; j < nblocks; j++)
            {   
                if (bitmap[i + j] != SIFS_UNUSED)
                {
                    available = false;
                    break;
                }
            }
            if (available)
            {
                for (SIFS_BLOCKID j = 0; j < nblocks; j++)
                {   
                    bitmap[i + j] = type;
                }
                return i;
            }
        }
    }
    return SIFS_ROOTDIR_BLOCKID;
}

void SIFS_freeblocks(void* volume, SIFS_BLOCKID firstblock, SIFS_BLOCKID nblocks)
{
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volume);
    for (SIFS_BLOCKID i = firstblock; i < firstblock + nblocks; i++)
    {
        bitmap[i] = SIFS_UNUSED;
    }
}

bool SIFS_hasentry(void* volume, SIFS_DIRBLOCK* directory, const char* entryname)
{
    for (int i = 0; i < directory->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volume, directory->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* dir = (SIFS_DIRBLOCK*)SIFS_getblock(volume, directory->entries[i].blockID);
            if (strcmp(dir->name, entryname) == 0)
            {
                return true;
            }
        }
        else if (type == SIFS_FILE)
        {
            SIFS_FILEBLOCK* file = (SIFS_FILEBLOCK*)SIFS_getblock(volume, directory->entries[i].blockID);
            if (strcmp(file->filenames[directory->entries[i].fileindex], entryname) == 0)
            {
                return true;
            }
        }
    }
    return false;
}

SIFS_FILEBLOCK* SIFS_getfileblock(void* volume, const void* md5, SIFS_BLOCKID* outBlockid)
{
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volume);
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volume);
    for (uint32_t i = 0; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_FILE)
        {
            SIFS_FILEBLOCK* block = (SIFS_FILEBLOCK*)SIFS_getblock(volume, i);
            if (memcmp(md5, block->md5, MD5_BYTELEN) == 0)
            {
                if (outBlockid != NULL)
                {
                    *outBlockid = i;
                }
                return block;
            }
        }
    }
    return NULL;
}