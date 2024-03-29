#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>

#include "../sifs.h"

void free_entrynames(char** entrynames, uint32_t nentries)
{
	if (entrynames != NULL) {
		for (int e = 0; e < nentries; ++e) {
			free(entrynames[e]);
		}
		free(entrynames);
	}
}
void print_root(void)
{
	char** log;
	uint32_t nentries;
	time_t modtime;
	if (SIFS_dirinfo("volume", "", &log, &nentries, &modtime) == 1)
	{
		SIFS_perror(NULL);
	}
	else
	{
		printf("nentries: %i | modtime: %s", nentries, ctime(&modtime));
		for (int i = 0; i < nentries; i++)
		{
			printf("%s\n", log[i]);
		}
	}

	free_entrynames(log, nentries);
}

// Invalid argument
void test_error_SIFS_EINVAL(void)
{
	printf("RUNNING TEST ERROR EINVAL\n");

	int i = SIFS_mkdir(NULL, "");
	if (i == 1 && SIFS_errno == SIFS_EINVAL)
	{
		printf("TEST PASSED\n");
	}
	else
		printf("TEST FAILED\n");

}

// Cannot create volume
void test_error_SIFS_ECREATE(void)
{

}

// No such volume
void test_error_SIFS_ENOVOL(void)
{
	printf("RUNNING TEST ERROR ENOVOL\n");

	int i = SIFS_mkdir("NON_EXISTENT_VOLUME", "HOME/HI");
	if (i == 1 && SIFS_errno == SIFS_ENOVOL)
	{
		printf("TEST PASSED\n");
	}
	else
		printf("TEST FAILED\n");
}

// No such file or directory entry
void test_error_SIFS_ENOENT(void)
{
	printf("RUNNING TEST ERROR ENOENT\n");

	remove("volume");
	SIFS_mkvolume("volume", 1024, 8);

	int i = SIFS_mkdir("volume", "FILE1/FILE2");
	if (i == 1 && SIFS_errno == SIFS_ENOENT)
	{
		printf("TEST PASSED\n");
	}
	else
		printf("TEST FAILED\n");
}

// Volume, file or directory already exists
void test_error_SIFS_EEXIST(void)
{
	printf("RUNNING TEST ERROR EEXIST\n");

	remove("volume");
	SIFS_mkvolume("volume", 1024, 8);

	SIFS_mkdir("volume", "FILE1");
	SIFS_mkdir("volume", "FILE1/FILE2");

	if (SIFS_mkdir("volume", "FILE1/FILE2/VID") == 1)
	{
		SIFS_perror(NULL);
	}
	int i = SIFS_mkdir("volume", "FILE1/FILE2/VID");
	if (i == 1 && SIFS_errno == SIFS_EEXIST)
	{
		printf("TEST PASSED\n");
	}
	else
	{
		printf("TEST FAILED %i\n", i);
		SIFS_perror(NULL);
	}
}

// Not a volume
void test_error_SIFS_ENOTVOL(void)
{
	printf("RUNNING TEST ERROR ENOTVOL\n");

	int i = SIFS_mkdir("tests/mkdir_test.c", "DIR");
	if (i == 1 && SIFS_errno == SIFS_ENOTVOL)
	{
		printf("TEST PASSED\n");
	}
	else
		printf("TEST FAILED\n");
}

// Not a directory
void test_error_SIFS_ENOTDIR(void)
{

}

// Not a file
void test_error_SIFS_ENOTFILE(void)
{

}

// Too many directory or file entries
void test_error_SIFS_EMAXENTRY(void)
{
	printf("RUNNING TEST ERROR EMAXENTRY\n");

	remove("volume");
	SIFS_mkvolume("volume", 1024, 64);
	
	for (int i = 0; i < 24; i++)
	{
		char directory[2] = "";
		directory[0] = 'A' + i;
		SIFS_mkdir("volume", directory);
	}

	int i = SIFS_mkdir("volume", "Z");
	if (i == 1 && SIFS_errno == SIFS_EMAXENTRY)
	{
		printf("TEST PASSED\n");
	}
	else
	{
		printf("TEST FAILED\n");
	}
}

// No space left on volume
void test_error_SIFS_ENOSPC(void)
{
	printf("RUNNING TEST ERROR ENOSPC\n");

	remove("volume");
	SIFS_mkvolume("volume", 1024, 8);

	for (int i = 0; i < 7; i++)
	{
		char directory[2] = "";
		directory[0] = 'A' + i;
		SIFS_mkdir("volume", directory);
	}

	int i = SIFS_mkdir("volume", "Z");
	if (i == 1 && SIFS_errno == SIFS_ENOSPC)
	{
		printf("TEST PASSED\n");
	}
	else
	{
		printf("TEST FAILED\n");
	}
}

// Memory allocation failed
void test_error_SIFS_ENOMEM(void)
{

}

// Not yet implemented
void test_error_SIFS_ENOTYET(void)
{

}

void test_nested_dir(void)
{
	printf("RUNNING TEST NESTED DIRECTORIES\n");

	remove("volume");
	SIFS_mkvolume("volume", 1024, 8);

	if (SIFS_mkdir("volume", "FILEA") == 1)
	{
		printf("TEST FAILED");
		return;
	}
	if (SIFS_mkdir("volume", "FILEA/FILEB") == 1)
	{
		printf("TEST FAILED");
		return;
	}
	if (SIFS_mkdir("volume", "FILEA/FILEB/IMAGES") == 1)
	{
		printf("TEST FAILED");
		return;
	}
	if (SIFS_mkdir("volume", "FILEA/FILEB/VIDEOS") == 1)
	{
		printf("TEST FAILED");
		return;
	}

	char** log;
	uint32_t nentries;
	time_t modtime;
	if (SIFS_dirinfo("volume", "FILEA/FILEB", &log, &nentries, &modtime) == 1)
	{
		printf("TEST FAILED");
		return;
	}
	else
	{
		bool passed = nentries == 2;
		char* reference[] = {
			"IMAGES", "VIDEOS"
		};
		for (int i = 0; i < nentries; i++)
		{
			passed = passed && (strcmp(reference[i], log[i]) == 0);
		}
		free_entrynames(log, nentries);
		if (passed)
		{
			printf("TEST PASSED\n");
		}
		else
		{
			printf("TEST FAILED\n");
		}
	}

	
}

int main(int argcount, char* argvalue[])
{
	test_error_SIFS_EINVAL();
	test_error_SIFS_ECREATE();
	test_error_SIFS_ENOVOL();
	test_error_SIFS_ENOENT();
	test_error_SIFS_EEXIST();
	test_error_SIFS_ENOTVOL();
	test_error_SIFS_ENOTDIR();
	test_error_SIFS_ENOTFILE();
	test_error_SIFS_EMAXENTRY();
	test_error_SIFS_ENOSPC();
	test_error_SIFS_ENOMEM();
	test_error_SIFS_ENOTYET();

	test_nested_dir();

	remove("volume");

	return 0;
}
