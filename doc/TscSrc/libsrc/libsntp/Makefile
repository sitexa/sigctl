#
# Makefile for libsntp
#

prjPath = ../..
#include $(prjPath)/Rules.make

GLOBAL_PREFIX ?= ERROR
GLOBAL_FLAG ?= 

VPATH = src
SOURCES = $(foreach dir, $(VPATH), $(wildcard $(dir)/*.c))
OBJS 	= $(SOURCES:%.c=%.o)

CROSS_COMPILE = arm-none-linux-gnueabi-

CC 	= $(CROSS_COMPILE)gcc
AR 	= $(CROSS_COMPILE)ar
RANLIB 	= $(CROSS_COMPILE)ranlib

CFLAG 	=-Wall -I./include
CFLAG += $(GLOBAL_FLAG)
CFLAG += -I$(prjPath)/include/net/netService

DFLAG 	=-DARCH_ARM  -g #-DSNTPDEBUG

CFLAGS 	=$(CFLAG) $(DFLAG) $(IFLAG)

TARGET 	= ./libnet_sntp.a 

all : $(TARGET) 

$(TARGET): $(OBJS)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@
	cp $@ $(prjPath)/$(GLOBAL_LIB) 

%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean :
	rm -f $(OBJS) *.bak $(TARGET)

