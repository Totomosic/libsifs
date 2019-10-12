#include "sifsutils.h"

// get information about a requested file
int SIFS_fileinfo(const char *volumename, const char *pathname,
		  size_t *length, time_t *modtime)
{
    if (volumename == NULL || pathname == NULL)
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

    // Find fileblock referenced by pathname
    SIFS_FILEBLOCK* fileblock = SIFS_getfile(volumename, result, count, NULL);
    if (fileblock == NULL)
    {
        // SIFS_errno set in SIFS_getfile()
        freesplit(result);
        return SIFS_FAILURE;
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
    return SIFS_SUCCESS;
}
