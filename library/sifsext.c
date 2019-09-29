#include "sifsext.h"
#include <stdio.h>

void SIFS_ls(const char* volumename, const char* dirname)
{
    size_t count;
    char** result = strsplit(dirname, SIFS_DIR_DELIMETER, &count);

    SIFS_DIRBLOCK* dir = SIFS_getdir(volumename, result, count, NULL);
    if (dir != NULL)
    {
        for (int i = 0; i < dir->nentries; i++)
        {
            SIFS_BIT type = SIFS_getblocktype(volumename, dir->entries[i].blockID);
            if (type == SIFS_DIR)
            {
                SIFS_DIRBLOCK* dirblock = (SIFS_DIRBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
                printf("DIR %s\n", dirblock->name);
                free(dirblock);
            }
            else if (type == SIFS_FILE)
            {
                SIFS_FILEBLOCK* file = (SIFS_FILEBLOCK*)SIFS_getblock(volumename, dir->entries[i].blockID);
                printf("FILE %s\n", file->filenames[dir->entries[i].fileindex]);
                free(file);
            }
        }
        free(dir);
    }

    freesplit(result);
}