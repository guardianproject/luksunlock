

CWD = $(shell pwd)
PROJECT_ROOT = $(CWD)/..
EXTERNAL_ROOT = $(PROJECT_ROOT)/external

# install root for built files
DESTDIR = $(EXTERNAL_ROOT)
# TODO try adding the Android-style /data/app.name here
prefix = /data/local
LOCAL := $(DESTDIR)$(prefix)


all: $(LOCAL)/lib/libcutils.so $(LOCAL)/lib/libpixelflinger.so

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


clean:
