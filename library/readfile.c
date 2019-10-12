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
        return SIFS_ERROR;
    }

    size_t count;
    char** result = strsplit(pathname, SIFS_DIR_DELIMETER, &count);
    if (result == NULL)
    {
        SIFS_errno = SIFS_ENOMEM;
        return SIFS_ERROR;
    }

    SIFS_FILEBLOCK* fileblock = SIFS_getfile(volumename, result, count, NULL);
    if (fileblock == NULL)
    {
        freesplit(result);
        return SIFS_ERROR;
    }

    size_t length = fileblock->length;
    char* buffer = (char*)malloc(length);
    if (buffer == NULL)
    {
        freesplit(result);
        free(fileblock);
        SIFS_errno = SIFS_ENOMEM;
        return SIFS_ERROR;
    }
    SIFS_VOLUME_HEADER header = SIFS_getvolumeheader(volumename);
    void* datablock = SIFS_getblocks(volumename, fileblock->firstblockID, SIFS_calcnblocks(&header, length));
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
    return SIFS_OK;
}
