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

void* SIFS_readvolume(const char* volumename, size_t offset, size_t length)
{
    struct stat fStat;
    if (stat(volumename, &fStat) != 0)
    {
        SIFS_errno = SIFS_ENOVOL;
        return NULL;
    }
    FILE* file = fopen(volumename, "rb");
    if (file == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return NULL;
    }
    void* headerData = malloc(sizeof(SIFS_VOLUME_HEADER));
    void* data = malloc(length);
    if (data == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return NULL;
    }
    fread(headerData, 1, sizeof(SIFS_VOLUME_HEADER), file);
    fseek(file, offset, SEEK_SET);
    fread(data, 1, length, file);
    
    SIFS_VOLUME_HEADER* header = (SIFS_VOLUME_HEADER*)headerData;
    size_t expectedLength = header->blocksize * header->nblocks + sizeof(SIFS_VOLUME_HEADER) + sizeof(SIFS_BIT) * header->nblocks;
    if (fStat.st_size != expectedLength)
    {
        SIFS_errno = SIFS_ENOTVOL;
        free(headerData);
        free(data);
        fclose(file);
        return NULL;
    }
    free(headerData);
    fclose(file);
    return data;
}

int SIFS_updatevolume(const char* volumename, size_t offset, const void* data, size_t nbytes)
{
    FILE* file = fopen(volumename, "rb+");
    if (file == NULL)
    {
        return 1;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(data, 1, nbytes, file);
    fclose(file);
    return 0;
}

SIFS_VOLUME_HEADER* SIFS_getvolumeheader(const char* volumename)
{
    return (SIFS_VOLUME_HEADER*)SIFS_readvolume(volumename, 0, sizeof(SIFS_VOLUME_HEADER));
}

SIFS_BIT* SIFS_getvolumebitmap(const char* volumename)
{
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
    if (header == NULL)
    {
        return NULL;
    }
    SIFS_BIT* ptr = (SIFS_BIT*)SIFS_readvolume(volumename, sizeof(SIFS_VOLUME_HEADER), header->nblocks * sizeof(SIFS_BIT));
    free(header);
    return ptr;
}

void SIFS_updatevolumebitmap(const char* volumename, const SIFS_BIT* bitmap, size_t length)
{
    if (length == 0)
    {
        SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
        if (header == NULL)
        {
            return;
        }
        length = header->nblocks;
        free(header);
    }
    SIFS_updatevolume(volumename, sizeof(SIFS_VOLUME_HEADER), (void*)bitmap, length);
}

void SIFS_updateblock(const char* volumename, SIFS_BLOCKID blockIndex, const void* data, size_t length)
{
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
    if (header == NULL)
    {
        return;
    }
    if (length == 0)
    {
        length = header->blocksize;
    }
    size_t offset = sizeof(SIFS_VOLUME_HEADER) + header->nblocks * sizeof(SIFS_BIT) + blockIndex * header->blocksize;
    SIFS_updatevolume(volumename, offset, data, length);
    free(header);
}

void* SIFS_getblock(const char* volumename, SIFS_BLOCKID blockIndex)
{
    return SIFS_getblocks(volumename, blockIndex, 1);
}

void* SIFS_getblocks(const char* volumename, SIFS_BLOCKID first, SIFS_BLOCKID nblocks)
{
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
    if (header == NULL)
    {
        return NULL;
    }
    size_t offset = sizeof(SIFS_VOLUME_HEADER) + sizeof(SIFS_BIT) * header->nblocks + header->blocksize * first;
    void* ptr = SIFS_readvolume(volumename, offset, header->blocksize * nblocks);
    free(header);
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
    if (dircount > 0 && ((strcmp(dirnames[0], ".") == 0) || (strcmp(dirnames[0], "~") == 0)))
    {
        dircount -= 1;
        dirnames += 1;
    }
    if (dircount == 0)
    {
        if (outBlockId != NULL)
        {
            *outBlockId = SIFS_ROOTDIR_BLOCKID;
        }
        return root;
    }
    return SIFS_finddir(volumename, root, dirnames, dircount, outBlockId);
}

SIFS_DIRBLOCK* SIFS_finddir(const char* volumename, SIFS_DIRBLOCK* dir, char** dirnames, size_t dircount, SIFS_BLOCKID* outBlockId)
{
    for (int i = 0; i < dir->nentries; i++)
    {
        SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
        if (type == SIFS_DIR)
        {
            SIFS_DIRBLOCK* block = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
            if (strcmp(block->name, dirnames[0]) == 0)
            {
                if (dircount == 1)
                {
                    if (outBlockId != NULL)
                    {
                        *outBlockId = dir->entries[i].blockID;
                    }
                    free(dir);
                    return block;
                }
                return SIFS_finddir(volumename, block, dirnames + 1, dircount - 1, outBlockId);
            }
            free(block);
        }
    }
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
    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, path, count - 1, NULL);
    if (dir == NULL)
    {
        return NULL;
    }
    char* filename = path[count - 1];
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
    if (SIFS_hasentry(volumename, dir, filename))
    {
        free(dir);
        SIFS_errno = SIFS_ENOTFILE;
        return NULL;
    }
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
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
    if (header == NULL)
    {
        return SIFS_ROOTDIR_BLOCKID;
    }
    if (nblocks > header->nblocks)
    {
        return SIFS_ROOTDIR_BLOCKID;
    }
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    for (size_t i = SIFS_ROOTDIR_BLOCKID + 1; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_UNUSED)
        {
            bool available = true;
            for (SIFS_BLOCKID j = 1; j < nblocks && (i + j < header->nblocks); j++)
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
                SIFS_updatevolumebitmap(volumename, bitmap, header->nblocks);
                free(bitmap);
                free(header);
                return i;
            }
        }
    }
    free(bitmap);
    free(header);
    return SIFS_ROOTDIR_BLOCKID;
}

void SIFS_freeblocks(const char* volumename, SIFS_BLOCKID firstblock, SIFS_BLOCKID nblocks)
{
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    if (bitmap == NULL)
    {
        return;
    }
    for (SIFS_BLOCKID i = firstblock; i < firstblock + nblocks; i++)
    {
        bitmap[i] = SIFS_UNUSED;
    }
    SIFS_updatevolumebitmap(volumename, bitmap, 0);
    free(bitmap);
}

bool SIFS_hasentry(const char* volumename, SIFS_DIRBLOCK* directory, const char* entryname)
{
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
    SIFS_VOLUME_HEADER* header = SIFS_getvolumeheader(volumename);
    SIFS_BIT* bitmap = SIFS_getvolumebitmap(volumename);
    if (header == NULL || bitmap == NULL)
    {
        SIFS_errno = SIFS_ENOVOL;
        return NULL;
    }
    for (uint32_t i = 0; i < header->nblocks; i++)
    {
        if (bitmap[i] == SIFS_FILE)
        {
            SIFS_FILEBLOCK* block = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, i);
            if (memcmp(md5, block->md5, MD5_BYTELEN) == 0)
            {
                if (outBlockid != NULL)
                {
                    *outBlockid = i;
                }
                free(header);
                free(bitmap);
                return block;
            }
            free(block);
        }
    }
    free(header);
    free(bitmap);
    return NULL;
}