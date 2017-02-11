/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#if defined(linux) || defined(Linux) || defined(__linux__) || defined(__linux) || defined(__gnu_linux__)
#include <compiler.h>
#endif

#if defined(__CYGWIN32__)
#define cpu_to_le32(x) (x)
#endif

#if defined(__WIN__)
#define cpu_to_le32(x) (x)
#endif

#if defined(__APPLE_CC__)
#if defined(__x86_64__) && defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define cpu_to_le32(x) (unsigned long) OSSwapHostToLittleInt32((uint32_t)x)
#else
#include <architecture/byte_order.h>
#define cpu_to_le32(x) NXSwapHostLongToLittle(x)
#endif
#endif

#define IMG_SIZE		(16*1024)
#define SPL_HEADER      "S5PC110 HEADER  "
struct spl_header {
	uint32_t spl_size;
	uint32_t reserved1;
	uint32_t spl_checksum;
	uint32_t reserved2;
};
#define SPL_HEADER_SIZE (sizeof(struct spl_header))
#define MAX_BUF_SIZE (IMG_SIZE-SPL_HEADER_SIZE)

#define FILE_PERM		(S_IRUSR | S_IWUSR | S_IRGRP	\
				| S_IWGRP | S_IROTH | S_IWOTH)
/*
 * Requirement for the fixed size SPL header:
 * IROM code reads first IMG_SIZE bytes from boot device. It then
 * calculates the checksum of (IMG_SIZE-SPL_HEADER_SIZE) bytes and compares
 * with data at spl_header.spl_checksum.
 *
 * Requirement for the variable size SPL header:

 * IROM code reads the spl_header to find out the size of the blob (total
 * size, header size included) and its checksum. Then it reads the rest of the
 * blob [i.e size - SPL_HEADER_SIZE bytes], calculates the
 * checksum and compares it with value read from the header.
 */

static const char *prog_name;

static void write_to_file(int ofd, void *buffer, int size)
{
	if (write(ofd, buffer, size) == size)
		return;

	fprintf(stderr, "%s: Failed to write to output file: %s\n",
		prog_name, strerror(errno));
	exit(EXIT_FAILURE);
}

/*
 * The argv is expected to include one optional parameter and two filenames:
 * [--vs] IN OUT
 *
 * --vs - turns on the variable size SPL mode
 * IN  - the u-boot SPL binary, usually u-boot-spl.bin
 * OUT - the prepared SPL blob, usually ${BOARD}-spl.bin
 *
 * This utility first reads the "u-boot-spl.bin" into a buffer. In case of
 * fixed size SPL the buffer size is exactly IMG_SIZE-SPL_HEADER_SIZE (such that
 * smaller u-boot-spl.bin gets padded with 0xff bytes, the larger than limit
 * u-boot-spl.bin causes an error). For variable size SPL the buffer size is
 * eqaul to size of the IN file.
 *
 * Then it calculates checksum of the buffer by just summing up all bytes.
 * Then
 *
 * - for fixed size SPL the spl_header is filled with the string SPL_HEADER and spl_header.spl_checksum is updated with the above calculated checksum in little endian format.
 *
 * - for variable size SPL the checksum and file size are stored in the
 *   spl_header structure (again, in little endian format).
 *
 * Finally the spl_header structure is written into the output file
 * followed by the buffer.
 */
int main(int argc, char **argv)
{
	unsigned char *buffer;
	int i, ifd, ofd;
	uint32_t checksum = 0;
	off_t	len;
	int	var_size_flag, read_size, count;
	struct stat stat;
	const int if_index = argc - 2; /* Input file name index in argv. */
	const int of_index = argc - 1; /* Output file name index in argv. */

	/* Strip path off the program name. */
	prog_name = strrchr(argv[0], '/');
	if (prog_name)
		prog_name++;
	else
		prog_name = argv[0];

	if ((argc < 3) ||
	    (argc > 4) ||
	    ((argc == 4) && strcmp(argv[1], "--vs"))) {
		fprintf(stderr, "Usage: %s [--vs] <infile> <outfile>\n",
			prog_name);
		exit(EXIT_FAILURE);
	}

	/* four args mean variable size SPL wrapper is required */
	var_size_flag = (argc == 4);

	ifd = open(argv[if_index], O_RDONLY);
	if (ifd < 0) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			prog_name, argv[if_index], strerror(errno));
		exit(EXIT_FAILURE);
	}

	ofd = open(argv[of_index], O_WRONLY | O_CREAT | O_TRUNC, FILE_PERM);
	if (ofd < 0) {
		fprintf(stderr, "%s: Can't open %s: %s\n",
			prog_name, argv[of_index], strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(ifd, &stat)) {
		fprintf(stderr, "%s: Unable to get size of %s: %s\n",
			prog_name, argv[if_index], strerror(errno));
		exit(EXIT_FAILURE);
	}

	len = stat.st_size;

	if (var_size_flag) {
		read_size = len;
		count = len;
	} else {
		if (len > MAX_BUF_SIZE) {
			fprintf(stderr,
				"%s: %s is too big (exceeds %ld bytes)\n",
				prog_name, argv[if_index], MAX_BUF_SIZE);
			exit(EXIT_FAILURE);
		}
		count = MAX_BUF_SIZE;
		read_size = len;
	}

	buffer = malloc(count);
	if (!buffer) {
		fprintf(stderr,
			"%s: Failed to allocate %d bytes to store %s\n",
			prog_name, count, argv[if_index]);
		exit(EXIT_FAILURE);
	}

	if (read(ifd, buffer, read_size) != read_size) {
		fprintf(stderr, "%s: Can't read %s: %s\n",
			prog_name, argv[if_index], strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* Pad if needed with 0xff to make flashing faster. */
	if (read_size < count)
		memset((char *)buffer + read_size, 0xff, count - read_size);

	for (i = 0, checksum = 0; i < count; i++)
		checksum += buffer[i];
	checksum = cpu_to_le32(checksum);

	struct spl_header vsh;
	if (var_size_flag) {
		uint32_t spl_size;
		memset(&vsh, 0, SPL_HEADER_SIZE);
		spl_size = cpu_to_le32(count + SPL_HEADER_SIZE);
		memcpy(&vsh.spl_size, &spl_size, sizeof(spl_size));
	} else {
		memcpy(&vsh, SPL_HEADER, SPL_HEADER_SIZE);
	}
	memcpy(&vsh.spl_checksum, &checksum, sizeof(checksum));

	write_to_file(ofd, &vsh, SPL_HEADER_SIZE);

	write_to_file(ofd, buffer, count);

	close(ifd);
	close(ofd);
	free(buffer);

	return EXIT_SUCCESS;
}
