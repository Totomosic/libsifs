#include "sifsutils.h"

// get information about a requested file
int SIFS_fileinfo(const char *volumename, const char *pathname,
		  size_t *length, time_t *modtime)
{
    if (volumename == NULL || pathname == NULL)
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

    SIFS_FILEBLOCK* fileblock = SIFS_getfile(volume, result, count, NULL);
    if (fileblock == NULL)
    {
        freesplit(result);
        free(volume);
        return 1;
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
    free(volume);
    SIFS_errno = SIFS_EOK;
    return 0;
}
