#include "sifs-internal.h"
#include <stdbool.h>

#define SIFS_DIR_DELIMETER '/'

// Returns vector of strings not including delimeter, last element is NULL
extern char** strsplit(const char* str, char delimeter, size_t* outCount);
extern void freesplit(char** strsplitresult);

extern int SIFS_readvolumeptr(const char* volumename, void* data, size_t offset, size_t length);
// Returns pointer to volume contents or NULL if it does not exist.
// Pointer should be freed when finished with
extern void* SIFS_readvolume(const char* volumename, size_t offset, size_t length);
// Updates volume's contents
extern int SIFS_updatevolume(const char* volumename, size_t offset, const void* data, size_t nbytes);

extern SIFS_VOLUME_HEADER SIFS_getvolumeheader(const char* volumename);
extern SIFS_BIT* SIFS_getvolumebitmap(const char* volumename);
extern SIFS_BLOCKID SIFS_calcnblocks(SIFS_VOLUME_HEADER* header, size_t nbytes);

extern void SIFS_updatevolumebitmap(const char* volumename, const SIFS_BIT* bitmap, size_t length);
extern void SIFS_updateblock(const char* volumename, SIFS_BLOCKID blockId, const void* data, size_t length);

// Returns a pointer to the beginning of block
extern void* SIFS_getblock(const char* volumename, SIFS_BLOCKID blockIndex);
extern void* SIFS_getblocks(const char* volumename, SIFS_BLOCKID first, SIFS_BLOCKID nblocks);
// Gets the root directory from volume
extern SIFS_DIRBLOCK* SIFS_getrootdir(const char* volumename);
// Gets the directory from volume, use "" or NULL for root directory
extern SIFS_DIRBLOCK* SIFS_getdir(const char* volumename, char** dirnames, size_t dircount, SIFS_BLOCKID* outBlockId);
// Recursively search directories
extern SIFS_DIRBLOCK* SIFS_finddir(const char* volumename, SIFS_DIRBLOCK* dir, char** dirnames, size_t dircount, SIFS_BLOCKID* outBlockId);
// Gets the frile from the volume
extern SIFS_FILEBLOCK* SIFS_getfile(const char* volumename, char** path, size_t count, SIFS_BLOCKID* outFileIndex);
// Finds the type of a block
extern SIFS_BIT SIFS_getblocktype(const char* volumename, SIFS_BLOCKID blockIndex);
// Returns index to first block id
extern SIFS_BLOCKID SIFS_allocateblocks(const char* volumename, SIFS_BLOCKID nblocks, SIFS_BIT type);
// Frees previously allocated blocks
extern void SIFS_freeblocks(const char* volumename, SIFS_BLOCKID firstblock, SIFS_BLOCKID nblocks);

// Returns true if the given directory has entryname as an entry (either file or directory)
extern bool SIFS_hasentry(const char* volumename, SIFS_DIRBLOCK* directory, const char* entryname);
// Returns a pointer to a file block if it already exists
extern SIFS_FILEBLOCK* SIFS_getfileblock(const char* volumename, const void* md5, SIFS_BLOCKID* outBlockId);