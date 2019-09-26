#include "sifs-internal.h"

// get information about a requested directory
int SIFS_dirinfo(const char *volumename, const char *pathname,
                 char ***entrynames, uint32_t *nentries, time_t *modtime)
{
    SIFS_errno	= SIFS_ENOTYET;
    return 1;
}
