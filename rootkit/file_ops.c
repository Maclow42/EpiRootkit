#include <linux/fs.h>
#include <linux/kernel.h>
#include "epirootkit.h"

// Function prototypes
char *read_file(char *filename, int *readed_size);


char *read_file(char *filename, int *readed_size){
	struct file *file = NULL;
	char *buf = NULL;
	loff_t pos = 0;
	size_t buf_size = STD_BUFFER_SIZE;
	size_t total_read = 0;
	ssize_t read_size;

	// Open the file for reading
	file = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(file)) {
		ERR_MSG("read_file: failed to open file %s\n", filename);
		*readed_size = -1;
		return NULL;
	}

	// Allocate initial memory for the buffer
	buf = kmalloc(buf_size, GFP_KERNEL);
	if (!buf) {
		filp_close(file, NULL);
		*readed_size = -1;
		return NULL;
	}

	// Read the file content character by character
	while ((read_size = kernel_read(file, buf + total_read, 1, &pos)) > 0) {
		total_read += read_size;

		// Resize the buffer if needed
		if (total_read >= buf_size) {
			char *new_buf = krealloc(buf, buf_size * 2, GFP_KERNEL);
			if (!new_buf) {
				kfree(buf);
				filp_close(file, NULL);
				*readed_size = -1;
				return NULL;
			}
			buf = new_buf;
			buf_size *= 2;
		}
	}

	if (read_size < 0) {
		kfree(buf);
		filp_close(file, NULL);
		*readed_size = -1;
		return NULL;
	}

	buf[total_read] = '\0';
	filp_close(file, NULL);
	*readed_size = total_read;
	return buf;
}