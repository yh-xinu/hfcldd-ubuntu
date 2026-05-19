#hfcldd Makefile for Ubuntu server
#
# Please copy this file as a "Makefile" 
# to the directory which contains "hfcldd" and "tools".
#
# For example:
# If you are looking tools/scripts/hfcgpl_makedeb0,
#   $ cp tools/scripts/hfcgpl_makedeb0 Makefile
# or if you are looking hfcgpl_makedeb0 in this directory,
#   $ cp hfcgpl_makedeb0 ../../Makefile
#
# and then, run below commands.
#
# usage:
#   $ sudo make kernelclean
#   $ sudo make kernelpreconfig
#   $ make clean
#   $ make
#   $ sudo dpkg -i hfcldd*.ubuntu.amd64.deb
#

# names
HFCLDD=hfcldd
HBATOOLS=hfcldd-tools

# versions
MODULEVERFILE=$(HFCLDD)/hfcl_modulever.h
LN=$(shell sed -n '/HFC_X8664_UBUNTU/=' $(MODULEVERFILE))
HFCLDDVER=$(shell sed -n '$(shell expr $(LN) + 1)s/\#define\s*HFC_MODULEVER\s*\"\(.*\)\".*/\1/p' $(MODULEVERFILE))
UBUNTUVER=Ubuntu
KERNVER=$(shell uname -r | sed "s/\(.*\)-\(.*\)/\1/")
SIVER=$(shell uname -r | sed "s/\(.*\)-\(.*\)/\2/")

# build variables
KERNEL_SRC ?= /lib/modules/`uname -r`/build
FLAG=HFC_UBUNTU
ADDITIONALCFLAGS=-D_HFC_NO_RASLOG -D$(FLAG)
MAKEDIR=hfcldd_make
FAKEHBATOOLS=$(HBATOOLS)
HEADERSDIR=$(FAKEHBATOOLS)/$(HFCLDD)
HEADERSNAME=hfcldd-headers
#FAKEHFCLDD=$(HFCLDD)_fake
# packaging variables
PKGNAME=hfcldd
PKGVER=3
ARCH=amd64
UBUNTU=ubuntu
PKGDIR=$(PKGNAME)-$(HFCLDDVER)-$(PKGVER)-$(KERNVER).$(UBUNTU).$(ARCH)
HEADERSTGZ=$(HEADERSNAME)-$(HFCLDDVER)-$(PKGVER)-$(KERNVER).$(UBUNTU).$(ARCH).tgz
HFCLDDSRC=$(HFCLDD)-$(HFCLDDVER)-$(PKGVER)-$(KERNVER).$(UBUNTU).$(ARCH).src
GPL_MAINTAINER=Yosuke Yamada <yosuke.yamada.no@hitachi.com>

# temporary variables
DATE=$(shell env LC_ALL=C LANGUAGE=C date +%Y%m%d)
CURDIR=$(shell pwd)
CHANGELOG=$(shell env LC_ALL=C LANGUAGE=C date +'%a %b %_d %Y')

all: srctar

directories:
	if [ ! -d $(PKGDIR)/lib/modules/$(KERNVER)-$(SIVER)/kernel/drivers/scsi ];then \
	mkdir -p $(PKGDIR)/lib/modules/$(KERNVER)-$(SIVER)/kernel/drivers/scsi;fi
	if [ ! -d $(PKGDIR)/opt/hitachi/drivers/hba ];then \
	mkdir -p $(PKGDIR)/opt/hitachi/drivers/hba;fi
	if [ ! -d $(MAKEDIR) ];then \
	mkdir -p $(MAKEDIR);fi
	if [ ! -d $(HFCLDDSRC)/$(HFCLDD) ];then \
	mkdir -p $(HFCLDDSRC)/$(HFCLDD);fi
	if [ ! -d $(HFCLDDSRC)/tools/install/DEBIAN ];then \
        mkdir -p $(HFCLDDSRC)/tools/install/DEBIAN;fi

kernelclean: kernellinks
	$(MAKE) -C $(KERNEL_SRC) clean

kernellinks:
	if [ ! -e $(KERNEL_SRC) -a -e /home/cvs/linux-ubuntu ];then ln -s /home/cvs/linux-ubuntu $(KERNEL_SRC);fi
	if [ ! -e $(KERNEL_SRC) -a -e $(KERNEL_SRC) ];then ln -s $(KERNEL_SRC) $(KERNEL_SRC);fi
	if [ ! -e $(KERNEL_SRC) -a -e /usr/src/$(KERNVER)-$(SIVER) ];then ln -s /usr/src/$(KERNVER)-$(SIVER) $(KERNEL_SRC);fi
	if [ ! -e $(KERNEL_SRC) -a -e /usr/src/$(KERNVER) ];then ln -s /usr/src/$(KERNVER) $(KERNEL_SRC);fi
	if [ ! -e $(KERNEL_SRC)/.config ];then gzip -c -d /proc/config.gz > $(KERNEL_SRC)/.config;fi
	if [ ! -e /lib/modules/$(KERNVER)-$(SIVER)/build ];then ln -sf $(KERNEL_SRC) /lib/modules/$(KERNVER)-$(SIVER)/build;fi

kernelpreconfig: kernellinks
	$(MAKE) -C $(KERNEL_SRC) oldconfig
	$(MAKE) -C $(KERNEL_SRC) prepare
	$(MAKE) -C $(KERNEL_SRC) modules_prepare
	$(MAKE) -C $(KERNEL_SRC) scripts/genksyms

$(HFCLDD)/*.c \
$(HFCLDD)/*.h \
$(HFCLDD)/Makefile \
Makefile \
$(PKGDIR)/lib/modules/$(KERNVER)-$(SIVER)/kernel/drivers/scsi/hfcldd.ko \
$(PKGDIR)/lib/modules/$(KERNVER)-$(SIVER)/kernel/drivers/scsi/hfcldd_conf.ko:

	$(MAKE) directories

	cp -f $(HFCLDD)/Makefile $(MAKEDIR)/Makefile
	sed -i "s/^OSVER0.*=.*/OSVER0  = $(KERNVER)-$(SIVER)/" $(MAKEDIR)/Makefile
	sed -i "s/^EXTRA_CFLAGS\(.*\)/EXTRA_CFLAGS\1 $(ADDITIONALCFLAGS)/" $(MAKEDIR)/Makefile

	cp -f $(HFCLDD)/*.c $(MAKEDIR)
	cp -f $(HFCLDD)/*.h $(MAKEDIR)
	$(MAKE) -C $(MAKEDIR)
	cp $(MAKEDIR)/$(KERNVER)-$(SIVER)/hfcldd.ko $(MAKEDIR)/$(KERNVER)-$(SIVER)/hfcldd_conf.ko $(PKGDIR)/lib/modules/$(KERNVER)-$(SIVER)/kernel/drivers/scsi/

tools/install/DEBIAN/control \
tools/install/DEBIAN/preinst \
tools/install/DEBIAN/postinst \
tools/install/COPYING \
tools/install/version.txt \
$(PKGDIR)/opt/hitachi/drivers/hba/version.txt \
$(PKGDIR)/DEBIAN:

	$(MAKE) directories
# copy a copyright file
	cp tools/install/COPYING $(PKGDIR)/opt/hitachi/drivers/hba/

# copy debian control files
	cp -Rf tools/install/DEBIAN $(PKGDIR)/

# modify control informations
	sed -i "s/^Package:.*/Package: $(PKGDIR)/" $(PKGDIR)/DEBIAN/control 
	 sed -i "s/^Version:.*/Version: $(HFCLDDVER)-$(PKGVER)-$(KERNVER)-$(SIVER)/" $(PKGDIR)/DEBIAN/control
	sed -i "s/^Maintainer:.*/Maintainer: $(GPL_MAINTAINER)/" $(PKGDIR)/DEBIAN/control
	sed -i "s/^ENASVERDIR=.*/ENASVERDIR=\"$(KERNVER)-$(SIVER)\"/" $(PKGDIR)/DEBIAN/postinst
	sed -i "s/^ENASVERDIR=.*/ENASVERDIR=\"$(KERNVER)-$(SIVER)\"/" $(PKGDIR)/DEBIAN/preinst
	sed -i "s/^Installed-Size:.*/Installed-Size: `du -sx --exclude DEBIAN $(PKGDIR)|awk '{print $$1}'`/" $(PKGDIR)/DEBIAN/control

# generate a version.txt
	cp tools/install/version.txt  $(PKGDIR)/opt/hitachi/drivers/hba/version_hfcldd.txt
	echo  "  $(UBUNTUVER)\n" >> $(PKGDIR)/opt/hitachi/drivers/hba/version_hfcldd.txt
	echo  "    $(ARCH) :" >> $(PKGDIR)/opt/hitachi/drivers/hba/version_hfcldd.txt
	echo  "        $(KERNVER)-$(SIVER)\n" >> $(PKGDIR)/opt/hitachi/drivers/hba/version_hfcldd.txt
	echo  "  * Version $(HFCLDDVER)         $(CHANGELOG)\n"\
	>> $(PKGDIR)/opt/hitachi/drivers/hba/version_hfcldd.txt

# set executable flags
	chmod 755 $(PKGDIR)/DEBIAN/postinst $(PKGDIR)/DEBIAN/postrm

$(PKGDIR).deb: \
 $(HFCLDD)/*.c \
 $(HFCLDD)/*.h \
 $(HFCLDD)/Makefile \
 Makefile \
 $(PKGDIR)/lib/modules/$(KERNVER)-$(SIVER)/kernel/drivers/scsi/hfcldd.ko \
 $(PKGDIR)/lib/modules/$(KERNVER)-$(SIVER)/kernel/drivers/scsi/hfcldd_conf.ko \
 tools/install/COPYING \
 tools/install/DEBIAN/* \
 $(PKGDIR)/DEBIAN



# make a debian package
	dpkg -b $(PKGDIR)

#headerstar: $(PKGDIR).deb
#	cp -f $(HFCLDD)/*.h $(HEADERSDIR)
#	tar czvf $(HEADERSTGZ) $(HEADERSDIR)

#srctar: headerstar
#	cp -Rf $(HFCLDD) tools $(FAKEHFCLDD)/$(HFCLDD)
#	cp tools/scripts/hfcgpl_makedeb0 $(FAKEHFCLDD)/$(HFCLDD)/Makefile
#	cd $(FAKEHFCLDD);tar czvf ../$(PKGDIR).src.tgz $(HFCLDD)

srctar: $(PKGDIR).deb
	cp -Rf $(HFCLDD)/*.h $(HFCLDDSRC)/$(HFCLDD)
	cp -Rf $(HFCLDD)/*.c $(HFCLDDSRC)/$(HFCLDD)
	cp -Rf $(MAKEDIR)/Makefile $(HFCLDDSRC)/$(HFCLDD)
	cp -f Makefile $(HFCLDDSRC)/
	cp -f tools/install/COPYING $(HFCLDDSRC)/tools/install/COPYING
	cp -Rf tools/install/DEBIAN/control $(HFCLDDSRC)/tools/install/DEBIAN/
	cp -Rf tools/install/DEBIAN/postinst $(HFCLDDSRC)/tools/install/DEBIAN/
	cp -Rf tools/install/DEBIAN/postrm $(HFCLDDSRC)/tools/install/DEBIAN/
	cp -Rf tools/install/DEBIAN/preinst $(HFCLDDSRC)/tools/install/DEBIAN/
	cp -f tools/install/version.txt $(HFCLDDSRC)/tools/install/
	cp Makefile $(HFCLDDSRC)/Makefile
	cp -f readme_src.txt $(HFCLDDSRC)/readme_src.txt
	cp -f readme_src_en.txt $(HFCLDDSRC)/readme_src_en.txt
#	cd $(FAKEHFCLDD);tar czvf ../$(PKGDIR).src.tgz $(HFCLDD) tools Makefile readme_src.txt readme_src_en.txt
		
clean:
	$(MAKE) -C hfcldd clean
	rm -rf $(MAKEDIR)
	rm -rf $(PKGDIR) 
	rm -rf $(FAKEHBATOOLS)
	rm -rf $(FAKEHFCLDD)
	rm -rf $(PKGDIR).deb
	rm -rf $(HEADERSTGZ)
#	rm -rf $(PKGDIR).src.tgz
	rm -rf $(HFCLDDSRC)
