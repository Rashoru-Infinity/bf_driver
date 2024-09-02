#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <string.h>
#include <stdint.h>
#include "bf.h"

// free memory
static void bf_machine_destroy(bf_machine_t *m)
{
    free(m->code_head);
    free(m->data_head);
    free(m->obuf);
    memset(m, 0, sizeof(bf_machine_t));
}

int main(int argc, char **argv)
{
    int bf_fd = -1;
    struct stat buf;
    bf_machine_t m;
    memset(&m, 0, sizeof(bf_machine_t));
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 0;
    }
    if (!(bf_fd = open(argv[1], O_RDONLY)))
        goto err;
    if (fstat(bf_fd, &buf) < 0)
        goto err;
    if (!(m.code_arr = calloc(sizeof(uint8_t), buf.st_size + 1)))
        goto err;
    m.code_head = m.code_arr;
    if (!(m.data_arr = calloc(buf.st_size, sizeof(uint8_t))))
        goto err;
    m.data_head = m.data_arr;
    if (!(m.obuf = calloc(buf.st_size, sizeof(uint8_t))))
        goto err;
    if (read(bf_fd, m.code_arr, buf.st_size) < 0)
        goto err;
    close(bf_fd);
    m.code_size = buf.st_size;
    m.machine_size = buf.st_size;
    m.obuf_size = buf.st_size;
	int fd;

    if ((fd = open("/dev/bf_machine0", O_RDWR)) < 0)
    {
        perror("open");
        return errno;
    }
    for (int i = 0;i < 5;i++)
    {
        if ((ioctl(fd, BF_INIT_MACHINE, &m)) < 0)
        {
            perror("BF_INIT_MACHINE");
            close(fd);
            return errno;
        }
        write(fd, m.code_arr, m.code_size);
        read(fd, m.obuf, m.code_size);
        printf("%s\n", m.obuf);
    }
    if (close(fd) != 0)
        perror("close");
	return 0;
err:
    fprintf(stderr, "%s\n", strerror(errno));
    bf_machine_destroy(&m);
    if (bf_fd != -1)
        close(bf_fd);
    return errno;
}
