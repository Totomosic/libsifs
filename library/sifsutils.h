#include "sifs-internal.h"
#include <stdbool.h>

#define SIFS_DIR_DELIMITER '/'

#define SIFS_SUCCESS     0
#define SIFS_FAILURE     1

// Splits str based on delimiter into a vector of strings that do not include the delimiter (free with freesplit())
extern char** strsplit(const char* str, char delimiter, size_t* outCount);
// Correctly frees the result from strsplit
extern void freesplit(char** strsplitresult);

// Reads the contents of the volume into data, returns SIFS_FAILURE on failure
extern int SIFS_readvolumeptr(const char* volumename, void* data, size_t offset, size_t length);
// Returns pointer to volume contents or NULL if it does not exist.
// Pointer should be freed when finished with
extern void* SIFS_readvolume(const char* volumename, size_t offset, size_t length);
// Updates volume's contents
extern int SIFS_updatevolume(const char* volumename, size_t offset, const void* data, size_t nbytes);

// Returns the header of the volume, returns SIFS_FAILURE on failure
extern int SIFS_getvolumeheader(const char* volumename, SIFS_VOLUME_HEADER* header);
// Returns a pointer to the beginning of the bitmap for the volume
extern SIFS_BIT* SIFS_getvolumebitmap(const char* volumename);
// Calculates the number of blocks required to represent nbytes
extern SIFS_BLOCKID SIFS_calcnblocks(SIFS_VOLUME_HEADER* header, size_t nbytes);

// Rewrites the bitmap back into the volume
extern void SIFS_updatevolumebitmap(const char* volumename, const SIFS_BIT* bitmap, size_t length);
// Rewrites a block back into the volume
extern void SIFS_updateblock(const char* volumename, SIFS_BLOCKID blockId, const void* data, size_t length);

// Returns a pointer to the beginning of block
extern void* SIFS_getblock(const char* volumename, SIFS_BLOCKID blockIndex);
// Returns a pointer to the beginning of a set of contiguous blocks
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
// Returns index to first block id, returns SIFS_ROOTDIR_BLOCKID on failure
extern SIFS_BLOCKID SIFS_allocateblocks(const char* volumename, SIFS_BLOCKID nblocks, SIFS_BIT type);
// Frees previously allocated blocks
extern void SIFS_freeblocks(const char* volumename, SIFS_BLOCKID firstblock, SIFS_BLOCKID nblocks);

// Returns true if the given directory has entryname as an entry (either file or directory)
extern bool SIFS_hasentry(const char* volumename, SIFS_DIRBLOCK* directory, const char* entryname);
// Returns a pointer to a file block only if it already exists (ie. has the same md5)
extern SIFS_FILEBLOCK* SIFS_getfileblock(const char* volumename, const void* md5, SIFS_BLOCKID* outBlockId);
