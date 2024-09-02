#ifndef DRIVER_H
#define DRIVER_H

#include <linux/ioctl.h>
#include <linux/types.h>

typedef struct machine_s
{
    char *data_arr;
    char *data_head;
    char *code_arr;
    char *code_head;
    char *obuf;
    char *obuf_head;
    char *obuf_offset;
    size_t code_size;
    size_t machine_size;
    size_t obuf_size;
} bf_machine_t;

#define BF_MACHINE 'B'
#define BF_INIT_MACHINE _IOW(BF_MACHINE, 1, bf_machine_t)

#endif /* DRIVER_H */
