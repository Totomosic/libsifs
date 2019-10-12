#include "sifsutils.h"

// get information about a requested file
int SIFS_fileinfo(const char *volumename, const char *pathname,
		  size_t *length, time_t *modtime)
{
    if (volumename == NULL || pathname == NULL)
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
    if (length != NULL)
    {
        *length = fileblock->length;
    }
    if (modtime != NULL)
    {
        *modtime = fileblock->modtime;
    }

    freesplit(result);
    free(fileblock);
    SIFS_errno = SIFS_EOK;
    return SIFS_OK;
}
