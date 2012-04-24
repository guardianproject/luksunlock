# Please install the following prerequisites (instructions for each follows):
# 	Android OS SDK: http://source.android.com/download
#
# Install and prepare the Android OS SDK ( http://source.android.com/download )
# on Debian or Ubuntu

### these modify the calling shell
# point pkg-config to the .pc files generated from these builds
export PKG_CONFIG_PATH=$(LOCAL)/lib/pkgconfig
# workaround for cross-compiling bug in autoconf
export ac_cv_func_malloc_0_nonnull=yes

CWD = $(shell pwd)
PROJECT_ROOT = $(CWD)
EXTERNAL_ROOT = $(PROJECT_ROOT)/external

# Android NDK setup
NDK_BASE ?=  /usr/local/android-ndk
NDK_PLATFORM_LEVEL ?= 9
NDK_SYSROOT=$(NDK_BASE)/platforms/android-$(NDK_PLATFORM_LEVEL)/arch-arm
NDK_UNAME := $(shell uname -s | tr '[A-Z]' '[a-z]')
NDK_TOOLCHAIN=$(NDK_BASE)/toolchains/arm-linux-androideabi-4.4.3/prebuilt/$(NDK_UNAME)-x86

# to use the real HOST tag, you need the latest libtool files:
# http://stackoverflow.com/questions/4594736/configure-does-not-recognize-androideabi
#HOST := arm-none-linux-gnueabi
HOST := arm-linux-androideabi

# install root for built files
DESTDIR = $(CWD)
# TODO try adding the Android-style /data/app.name here
prefix = /data/local
LOCAL := $(DESTDIR)$(prefix)

PATH := ${PATH}:$(NDK_TOOLCHAIN)/bin:$(LOCAL)/bin

CC := $(NDK_TOOLCHAIN)/bin/arm-linux-androideabi-gcc --sysroot=$(NDK_SYSROOT)
CXX := $(NDK_TOOLCHAIN)/bin/arm-linux-androideabi-g++
CPP := $(NDK_TOOLCHAIN)/bin/arm-linux-androideabi-cpp
LD := $(NDK_TOOLCHAIN)/bin/arm-linux-androideabi-ld
AR := $(NDK_TOOLCHAIN)/bin/arm-linux-androideabi-ar
RANLIB := $(NDK_TOOLCHAIN)/bin/arm-linux-androideabi-ranlib
STRIP := $(NDK_TOOLCHAIN)/bin/arm-linux-androideabi-strip \
	--strip-unneeded -R .note -R .comment

CFLAGS = -DANDROID -I$(LOCAL)/include
LDFLAGS = -L$(LOCAL)/lib -Wl,--rpath,$(LOCAL)/lib
#ALL_LDFLAGS = -Wl,--entry=main,-rpath=$(ANDROID_NDK_ROOT)/build/platforms/android-$(NDK_PLATFORM_VER)/arch-arm/usr/lib,-dynamic-linker=/system/bin/linker -L$(NDK_SYSROOT)/usr/lib  -nostdlib -lc -ldl
#LIBS = -lc -ldl



all: $(LOCAL)/lib/libcutils.so $(LOCAL)/lib/libpixelflinger.so $(LOCAL)/lib/libhardware_legacy.so

.PHONY: clean

# these files should come from your phone or emulator.  They are part of the
# standard Android system, but the NDK does not provide them because they are
# private, internal libs

$(LOCAL)/lib:
	install -d $(LOCAL)/lib

$(LOCAL)/lib/libcutils.so: $(LOCAL)/lib
	adb pull /system/lib/libcutils.so $(LOCAL)/lib/
$(LOCAL)/lib/libpixelflinger.so:
	adb pull /system/lib/libpixelflinger.so $(LOCAL)/lib/
$(LOCAL)/lib/libhardware_legacy.so:
	adb pull /system/lib/libhardware_legacy.so $(LOCAL)/lib/



#------------------------------------------------------------------------------#
# libpng

libpng/config.sub: config.sub
	cp config.sub libpng/config.sub

libpng/config.guess: config.guess
	cp config.guess libpng/config.guess

libpng/configure: libpng/configure.ac
	cd libpng && ./autogen.sh

libpng/Makefile: libpng/config.sub libpng/config.guess libpng/configure
	cd libpng && \
		./configure \
			CC="$(CC)" \
			AR=$(AR) \
			RANLIB=$(RANLIB) \
			CFLAGS="$(CFLAGS)" \
			LDFLAGS="$(LDFLAGS)" \
			--enable-static \
			--with-gnu-ld \
			--host=$(HOST)

libpng-build-stamp: libpng/Makefile
	$(MAKE) -C libpng
	touch libpng-build-stamp

libpng-build: libpng-build-stamp

$(LOCAL)/lib/libpng.a: libpng-build
	$(MAKE) -C libpng DESTDIR=$(DESTDIR) prefix=$(prefix) install

libpng-install: $(LOCAL)/lib/libpng.a

libpng-clean:
	-rm -f libpng-build-stamp
	$(MAKE) -C libpng clean



#------------------------------------------------------------------------------#
# clean

clean: libpng-clean
	rm -f $(LOCAL)/lib/libcutils.so $(LOCAL)/lib/libpixelflinger.so
