#ifndef __MMAP_FILE_H__
#define __MMAP_FILE_H__

#include <stdbool.h>

struct mmap_file_context;

struct mmap_file_context *mmap_file_open(const char *filename, bool allow_write);
struct mmap_file_context *mmap_file_create(const char *filename, size_t len);
void mmap_file_close(struct mmap_file_context *mmap_file);

void *mmap_file_ptr(struct mmap_file_context *mmap_file);
size_t mmap_file_len(struct mmap_file_context *mmap_file);

#endif /* __MMAP_FILE_H__ */
