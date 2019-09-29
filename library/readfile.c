#include "sifsutils.h"
#include <string.h>
#include <stdio.h>

// read the contents of an existing file from an existing volume
int SIFS_readfile(const char *volumename, const char *pathname,
		  void **data, size_t *nbytes)
{
    if (volumename == NULL || pathname == NULL || data == NULL)
    {
        SIFS_errno = SIFS_EINVAL;
        return 1;
    }

    size_t count;
    char** result = strsplit(pathname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }

    SIFS_FILEBLOCK* fileblock = SIFS_getfile(volumename, result, count, NULL);
    if (fileblock == NULL)
    {
        freesplit(result);
        return 1;
    }

    size_t length = fileblock->length;
    char* buffer = (char*)malloc(length);
    if (buffer == NULL)
    {
        freesplit(result);
        free(fileblock);
        SIFS_errno = SIFS_ENOMEM;
        return 1;
    }
    void* datablock = SIFS_getblock(volumename, fileblock->firstblockID);
    memcpy(buffer, datablock, length);
    *data = buffer;
    if (nbytes != NULL)
    {
        *nbytes = length;
    }

    freesplit(result);
    free(datablock);
    free(fileblock);
    SIFS_errno = SIFS_EOK;
    return 0;
}
