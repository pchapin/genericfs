###########################################################################
# FILE   : Makefile
# SUBJECT: Basic makefile for building a Linux module.
#
# This file is designed to work with the 3.x series of kernels. It may also
# work with other kernel versions.
###########################################################################

# If the KERNEL environment variable is not set, use the running kernel.
KERNEL ?= $(shell uname -r)

all:
	make -C /lib/modules/$(KERNEL)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KERNEL)/build M=$(PWD) clean
