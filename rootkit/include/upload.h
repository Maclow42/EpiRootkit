#ifndef EPI_UPLOAD_H
#define EPI_UPLOAD_H

#include <linux/types.h>

extern bool receiving_file;
extern char *upload_buffer;
extern char *upload_path;
extern long upload_size;
extern long upload_received;

int handle_upload_chunk(const char *data, size_t len);
int start_upload(const char *path, long size);

#endif // EPI_UPLOAD_H
