#ifndef FILEREAD_H
#define FILEREAD_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static inline FILE * open_file (const char * filename)
{
	FILE * f = fopen(filename, "rb");
	if (f == NULL) {
		return NULL;
	}	
	return f;
}

static inline long file_size (const char * filename)
{
	FILE * f = open_file (filename);
	if (f == NULL) return -1;

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	fclose(f);

	return size;
}

static inline char * read_file (const char * filename)
{	
	FILE * f = open_file (filename);

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *buf = malloc(size + 1);
	if (!fread(buf, 1, size, f)) 
	{
		fclose(f);
		return NULL;
	}
	fclose(f);

	/* End with null character */
	buf[size] = 0;

	return buf;
}

#endif