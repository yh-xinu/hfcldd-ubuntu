##########################################################
#
# Makefile for Hitachi FC-Adapter Driver
#
##########################################################
# --- How to build and use Hitachi FC-Adapter Driver ---
#
# 1. Set your kernel src directory.
#    Example:
#        -bash-4.1# export KERNEL_SRC=/usr/src/kernels/3.10.13-101.fc18.x86_64/
#
# 2. Run "make" to build driver binary files(.ko files).
#    Example:
#        -bash-4.1# make
#
# 3. Run "insmod" to insert drivers into the Linux Kernel.
#    Example:
#        -bash-4.1# insmod /lib/modules/`uname -r`/kernel/drivers/scsi/scsi_tgt.ko
#        -bash-4.1# insmod /lib/modules/`uname -r`/kernel/drivers/scsi/scsi_transport_fc.ko
#        -bash-4.1# insmod ./`uname -r`/hraslog_link.ko
#        -bash-4.1# insmod ./`uname -r`/hfcldd_conf.ko
#        -bash-4.1# insmod ./`uname -r`/hfcldd.ko
#
##########################################################

KERNEL_SRC ?= /lib/modules/`uname -r`/build

#EXTRA_CFLAGS := -D_LINUX_COM -D__LINUX
#EXTRA_CFLAGS := -D_LINUX_COM -D__LINUX -DSYSFS_SUPPORT -DHFC_HCP -H
EXTRA_CFLAGS := -D_LINUX_COM -D__LINUX -DSYSFS_SUPPORT -D_HFC_FX_HW_CHECK_SKIP  -DHFC_RHEL7 -DHFC_X8664_UBUNTU -DHFC_UBUNTU -D_HFC_NO_RASLOG -DHFC_UBUNTU
#EXTRA_CFLAGS := -D_LINUX_COM -D__LINUX -DSYSFS_SUPPORT -D_HFC_FX_HW_CHECK_SKIP  -DHFC_X8664_VANILLA -D_HFC_DEBUG -D_HFC_MSGLVL3 -Werror

OSVER0  = 3.19.0-25-generic

HOSTTYPE := $(shell uname -m)

MODS = hraslog_link.ko hfcldd_conf.ko hfcldd.ko
OBJS = hraslog_link.o hfcldd_conf.o hfcldd.o
TARGET = $(MODS)

.PHONY: allup0

all : hraslog_linkup0 hfcldd_confup0 hfclddup0 moveup0

allup0 : hraslog_linkup0 hfcldd_confup0 hfclddup0 moveup0

hraslog_linkup0 : hraslog_link.c
	make -C $(KERNEL_SRC) M=`pwd` V=1 modules  >> result.log  2>> result.log 

hfcldd_confup0 : hfcldd_conf.c
	make -C $(KERNEL_SRC) M=`pwd` V=1 modules  >> result.log  2>> result.log 

hfclddup0 : hfcl_conf.c hfcl_detect.c hfcl_handler.c hfcl_diag.c hfcl_strategy.c hfcl_timer_recovery.c hfcl_top.c hfcl_ioctl.c hfcl_version.c hfcl_mlpf.c hfcl_detect_fx.c hfcl_handler_fx.c hfcl_diag_fx.c hfcl_strategy_fx.c hfcl_timer_recovery_fx.c hfcl_top_fx.c hfcl_ioctl_fx.c hfcl_mlpf_fx.c hfcl_npiv_fx.c
	make -C $(KERNEL_SRC) M=`pwd` V=1 modules  >> result.log  2>> result.log

moveup0 : 
	mkdir -p    $(OSVER0)
	mv ./*.o    $(OSVER0)/
	mv ./*.ko   $(OSVER0)/
	mv ./.*.cmd $(OSVER0)/
	mv ./*.log  $(OSVER0)/

clean : 
	rm -rf $(OSVER0)
	rm -f ./*.o
	rm -f ./*.ko
	rm -f ./.*.cmd
	rm -f ./*.log
	rm -f ./*.mod.c
	rm -f ./*.order

obj-m:= $(OBJS)

hfcldd-objs := hfcl_conf.o hfcl_detect.o hfcl_handler.o hfcl_diag.o hfcl_strategy.o hfcl_timer_recovery.o hfcl_top.o hfcl_ioctl.o hfcl_version.o hfcl_mlpf.o hfcl_detect_fx.o hfcl_handler_fx.o hfcl_diag_fx.o hfcl_strategy_fx.o hfcl_timer_recovery_fx.o hfcl_top_fx.o hfcl_ioctl_fx.o hfcl_mlpf_fx.o hfcl_npiv_fx.o

clean-files := *.o *.ko *.mod.c
