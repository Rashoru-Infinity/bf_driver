DRIVER_SRCS	= bf_driver.c
obj-m := $(DRIVER_SRCS:.c=.o)
APP_SRCS	= bf_application.c
APP_OBJS	= $(APP_SRCS:.c=.o)
KBUILD_CFLAGS	+= -g -Wall
CC	= gcc
APP_TARGET	= bf_application

all: $(TARGET)
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
.c.o:
	$(CC) -g -c $< -o $@
$(TARGET):	$(APP_OBJS)
	$(CC) $(APP_OBJS) -o $(APP_TARGET)
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm -f $(APP_TARGET)
	rm -f $(APP_OBJS)
