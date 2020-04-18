/* iQ-i2C helper ultility for the Linux system. Use normal i2c commands, you can read and write i2c registers
*  using this program. For further help check usage function.
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>

#include <linux/i2c-dev.h>

static void usage (const char *argv0) {

	fprintf(stderr,
		"Welcome to iQ-i2c helper utility"
		" Usages:\n"
		" Read register:\n"
		"   %s [-opts] read <bus> <device> <register> <length> [raw]\n"
		"     'raw' will dump the data as is, without this\n"
		"     option it will be printed in hex.\n"
		"\n"
		" Write register:\n"
		"   %s [-opts] write <bus> <device> <register>|-> "
		"{<path>|<hexbytes>}\n"
		"\n"
		"\n"
		"  <bus>      - i2c-bus id 1-4\n"
		"  <device>   - i2c device address in hex\n"
		"  <register> - i2c device address in hex (any length)\n"
		"               if specified as '-' the register is ignored.\n"
		"               same format as <hexbytes>\n"
		"  <length>   - length in bytes to read\n"
		"  raw        - print data as is, else print as hex\n"
		"  <path>     - file path with binary data\n"
		"  @<path>    - file path with hex data\n"
		"  <hexbytes> - In the format hh x N, i.e:\n"
		"               a1b2c3d4ffee02\n"
		"\n"
		"  [-opts] options:\n"
		"     -v      - verbose\n"
		"     -r      - raw binary output (not hex)\n"
		"     -f      - force device access even if locked by driver\n"
		"\n",
		argv0, argv0);
	exit(1);
}

static int rd_hex2bin (const char *hexstr, int inlen, char *dst, int dstlen) {
    const char *end = inlen == -1 ? (const char *)UINTPTR_MAX : hexstr+inlen;
    const char *s = hexstr;
    char *d = dst;
    char *dend = dst + dstlen;
    int state = 0;
    static const char ignore[256] = {
        [' '] = 1, ['\t'] = 1, ['.'] = 1,  [':'] = 1,
        ['\n'] = 1, ['\r'] = 1,
    };

    while (*s && s < end && d < dend) {
        char c;

        if (ignore[(int)*s]) {
            s++;
            continue;
        }

        if (*s >= '0' && *s <= '9')
            c = *s - '0';
        else if (*s >= 'A' && *s <= 'F')
            c = *s - 'A' + 10;
        else if (*s >= 'a' && *s <= 'f')
            c = *s - 'a' + 10;
        else
            break;

        if (state++ & 1)
            *(d++) |= c;
        else
            *d = c << 4;

        s++;
    }

    return (int)(d-dst);
}

/* Simple hex to binary converter */
static int hex2bin (const char *hexstr, char *dst, int dstlen) {

	if (!strcmp(hexstr, "-")) /* ignore */
		return 0;

	if (!strncmp(hexstr, "0x", 2))
		hexstr += 2;

	if (strlen(hexstr) == 1) {
		/* single digit, pad it. */
		char tmp[3] = { '0', *hexstr, '\0' };
		hexstr = tmp;
	}

	return rd_hex2bin(hexstr, -1, dst, dstlen);
}

static int file_size(const char *path) {
    struct stat buf;
    if (stat(path, &buf) == -1)
        return -1;

    return buf.st_size;
}

static char *file_read(const char *path, int *len) {
    char *data = NULL;
    int flen = file_size(path);
    if (flen < 0)
        return NULL;

    int fd = open(path, O_RDONLY);
    if (fd == -1)
        return NULL;

    data = malloc(flen);
    if (!data) {
        close(fd);
        return NULL;
    }

    int rlen = read(fd, data, flen);
    if (rlen != flen) {
        free(data);
        close(fd);
        return NULL;
    }

    close(fd);
    if (len)
        *len = rlen;

    return data;
}

static void hexdump(const char *data, size_t len) {
    int offset;
    int i;
    fprintf(stderr, "Read hexdump (%zu bytes):\n", len);
    for(offset = 0; offset < len; offset += 16) {
        fprintf(stderr, "%08x: ", offset);
        for(i = 0; i < 16 && (offset + i) < len; ++i)
            fprintf(stderr, "%02x ", data[i]);
        fprintf(stderr, "%*s", 49-i*3, "");
        for(i = 0; i < 16 && (offset + i) < len; ++i)
            fprintf(stderr, "%c", isprint(data[i]) ? data[i] : '.');
        fprintf(stderr, "\n");
    }
}

int main (int argc, char **argv) {
	char path[32];
	int fd;
	int bus;
	int addr;
	int len;
	int size = 512*1024;
	ssize_t r;
	int offset;
	int ai=1;
	const char *mode;
	int raw = 0;
	char *buf = NULL;
	char *outbuf;
	int outlen = 0;
	int force = 0;

	if (argc == 1)
		usage(argv[0]);

	while (ai < argc && argv[ai][0] == '-') {
		const char *s = &argv[ai][1];
		while (*s) {
			switch (*s)
			{
			case 'r':
				raw = 1;
				break;
			case 's':
				size = atoi(optarg);
				break;
			case 'f':
				force = 1;
				break;
			default:
				usage(argv[0]);
			}
			s++;
		}
		ai++;
	}

	if (ai + 3 >= argc)
		usage(argv[0]);

	mode = argv[ai++];
	bus = atoi(argv[ai++]);
	addr = strtoul(argv[ai++], NULL, 16);

	offset = 0;

	snprintf(path, sizeof(path), "/dev/i2c-%i", bus);
	/* remaining arguments: <register> <length> */
	if (!strcmp(mode, "read")) {
		if (ai + 2 > argc)
			usage(argv[0]);

		buf = malloc(size);
		offset += hex2bin(argv[ai++], buf+offset, size-offset);
		outlen = strtoul(argv[ai++], NULL, 0);

		fprintf(stderr, "%% %s: device 0x%x: %s %i (write %i) bytes\n",
			path, addr, mode, outlen, offset);

	} else if (!strcmp(mode, "write")) {
		/* Remaining arguments: {<path>|<hexbytes>}.. */
		if (ai + 1 > argc)
			usage(argv[0]);

		buf = malloc(size);

		/* Just accumulate a series offset files or hexbytes into the buf*/
		while (ai < argc) {
			int as_hex = 0;
			const char *arg = argv[ai];

			if (*arg == '@') {
				arg++;
				as_hex = 1;
			}

			if (offset >= size) {
				fprintf(stderr, "%% Out of buffer space: "
					"set it with -s <size> option\n");
				exit(1);
			}

			if ((len = file_size(arg)) == -1) {
				/* <hexbytes>, not <path> */
				as_hex = 1;
				offset += hex2bin(arg, buf+offset, size-offset);
			} else {
				char *tmp;

				/* <path>, read contents into buf. */
				if (!(tmp = file_read(arg, &len))) {
					fprintf(stderr,
						"%% Error: couldn't read file "
						"%s: %s\n",
						arg, strerror(errno));
					exit(1);
				}

				if (offset + len > size) {
					fprintf(stderr,
						"%% Error: file %s (%i bytes) "
						"would overflow buffer of "
						"size %i\n",
						arg, len, size);
					exit(1);
				}

				if (as_hex) {
					len = hex2bin(tmp, buf+offset, size-offset);
				} else {
					memcpy(buf+offset, tmp, len);
				}
				offset += len;

				free(tmp);
			}

			ai++;
		}

		fprintf(stderr, "%% %s: device 0x%x: %s %i bytes\n", path, addr, mode, offset);
	} else
		usage(argv[0]);


	if ((fd = i2c_init(path, addr, force)) == -1)
		exit(1);

	if (!strcmp(mode, "write")) {

		if ((r = i2c_write_buf(fd, buf, offset)) == -1) {
			fprintf(stderr, "%% i2c write failed: %s\n", strerror(errno));
			exit(1);
		}
 	} else if (!strcmp(mode, "read")) {

		outbuf = malloc(outlen);
		if ((r = i2c_read_buf(fd, addr, buf, offset,
				      outbuf, outlen)) == -1) {
			fprintf(stderr, "%% i2c read failed: %s\n", strerror(errno));
			exit(1);
		}

		if (raw)
			write(STDOUT_FILENO, outbuf, r);
		else
			hexdump(outbuf, r);
	}

	close(fd);

	return 0;
}
