#include "sifs-internal.h"
#include <stdbool.h>

#define SIFS_DIR_DELIMETER '/'

// Returns vector of strings not including delimeter, last element is NULL
extern char** strsplit(const char* str, char delimeter, size_t* outCount);
extern void freesplit(char** strsplitresult);

// Returns pointer to volume contents or NULL if it does not exist.
// Pointer should be freed when finished with
extern void* SIFS_readvolume(const char* volumename, size_t* outSize);
// Updates volume's contents
extern int SIFS_updatevolume(const char* volumename, size_t offset, const void* data, size_t nbytes);
// Completely rewrites volume
extern int SIFS_rewritevolume(const char* volumename, void* volumedata);

extern SIFS_VOLUME_HEADER* SIFS_getvolumeheader(void* volume);
extern SIFS_BIT* SIFS_getvolumebitmap(void* volume);

// Returns a pointer to the beginning of block
extern void* SIFS_getblock(void* volume, SIFS_BLOCKID blockIndex);
// Gets the root directory from volume
extern SIFS_DIRBLOCK* SIFS_getrootdir(void* volume);
// Gets the directory from volume, use "" or NULL for root directory
extern SIFS_DIRBLOCK* SIFS_getdir(void* volume, char** dirnames, size_t dircount);
// Recursively search directories
extern SIFS_DIRBLOCK* SIFS_finddir(void* volume, SIFS_DIRBLOCK* dir, char** dirnames, size_t dircount);
// Gets the frile from the volume
extern SIFS_FILEBLOCK* SIFS_getfile(void* volume, char** path, size_t count, SIFS_BLOCKID* outFileIndex);
// Finds the type of a block
extern SIFS_BIT SIFS_getblocktype(void* volume, SIFS_BLOCKID blockIndex);
// Returns index to first block id
extern SIFS_BLOCKID SIFS_allocateblocks(void* volume, SIFS_BLOCKID nblocks, SIFS_BIT type);
// Frees previously allocated blocks
extern void SIFS_freeblocks(void* volume, SIFS_BLOCKID firstblock, SIFS_BLOCKID nblocks);

// Returns true if the given directory has entryname as an entry (either file or directory)
extern bool SIFS_hasentry(void* volume, SIFS_DIRBLOCK* directory, const char* entryname);
// Returns a pointer to a file block if it already exists
extern SIFS_FILEBLOCK* SIFS_getfileblock(void* volume, const void* md5, SIFS_BLOCKID* outBlockId);