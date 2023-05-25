#ifndef FILEREAD_H
#define FILEREAD_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline FILE * open_file (const char * filename);
static inline long   file_size (const char * filename);
static inline char * read_file (const char * filename);

#endif