#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>

#include "mmap_file.h"

struct mmap_file_context {
	int fd;
	void *ptr;
	size_t len;
	bool create;
};

struct mmap_file_context *mmap_file_open(const char *filename, bool allow_write)
{
	struct mmap_file_context *mmap_file;
	struct stat statbuf;
	int flags, prot, ret;

	mmap_file = (struct mmap_file_context *)malloc(sizeof(struct mmap_file_context));
	if (!mmap_file) {
		fprintf(stderr, "Failed to malloc mmap_file context\n");
		return NULL;
	}

	*mmap_file = (struct mmap_file_context) {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.create = false,
	};

	flags = allow_write ? O_RDWR : O_RDONLY;
	mmap_file->fd = open(filename, flags);
	if (mmap_file->fd == -1) {
		fprintf(stderr, "Failed to open file '%s'\n", filename);
		goto fail;
	}

	ret = fstat(mmap_file->fd, &statbuf);
	if (ret == -1) {
		fprintf(stderr, "Failed to stat file '%s'\n", filename);
		goto fail;
	}

	prot = PROT_READ;
	if (allow_write)
		prot |= PROT_WRITE;

	flags = allow_write ? MAP_SHARED : MAP_PRIVATE;

	mmap_file->ptr = mmap(NULL, statbuf.st_size, prot, flags, mmap_file->fd, 0);
	if (mmap_file->ptr == MAP_FAILED) {
		perror("mmap");
		fprintf(stderr, "Failed to mmap file '%s'\n", filename);
		goto fail;
	}
	mmap_file->len = statbuf.st_size;

	return mmap_file;

fail:
	mmap_file_close(mmap_file);
	return NULL;
}

struct mmap_file_context *mmap_file_create(const char *filename, size_t len)
{
	struct mmap_file_context *mmap_file;
	int ret;

	assert(len > 0);

	mmap_file = (struct mmap_file_context *)malloc(sizeof(struct mmap_file_context));
	if (!mmap_file) {
		fprintf(stderr, "Failed to malloc mmap_file context\n");
		return NULL;
	}

	*mmap_file = (struct mmap_file_context) {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.create = true,
	};

	mmap_file->fd = open(filename, O_CREAT | O_RDWR, 0644);
	if (mmap_file->fd == -1) {
		fprintf(stderr, "Failed to create file '%s'\n", filename);
		goto fail;
	}

	ret = ftruncate(mmap_file->fd, len);
	if (ret == -1) {
		fprintf(stderr, "Failed to resize '%s to '%zu' bytes\n", filename, len);
		goto fail;
	}

	mmap_file->ptr = mmap(NULL, len, PROT_READ|PROT_WRITE, MAP_SHARED, mmap_file->fd, 0);
	if (mmap_file->ptr == MAP_FAILED) {
		perror("mmap");
		fprintf(stderr, "Failed to mmap file '%s'\n", filename);
		goto fail;
	}
	mmap_file->len = len;

	return mmap_file;

fail:
	mmap_file_close(mmap_file);
	return NULL;
}

void mmap_file_close(struct mmap_file_context *mfile)
{
	assert(mfile);

	if (mfile->ptr && mfile->len > 0) {
		if (mfile->create)
			msync(mfile->ptr, mfile->len, MS_SYNC);

		munmap(mfile->ptr, mfile->len);
	}

	if (mfile->fd != -1)
		close(mfile->fd);

	free(mfile);
}

void *mmap_file_ptr(struct mmap_file_context *mfile)
{
	assert(mfile);

	return mfile->ptr;
}

size_t mmap_file_len(struct mmap_file_context *mfile)
{
	assert(mfile);

	return mfile->len;
}
