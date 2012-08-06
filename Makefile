NDK = ${HOME}/src/android/android-ndk-r6
TOOLCHAIN= arm-linux-androideabi
TOOLCHAINVERS = 4.4.3

### End of config area

SRCS = pgdn.c

CPPFLAGS+= -D_GNU_SOURCE
CPPFLAGS+= -D_XOPEN_SOURCE
CPPFLAGS+= -I.
CPPFLAGS+= -I./linux-2.6.35.3/include

CFLAGS+= -O2 -g -Wall -Wno-unused-parameter

### Android standalone toolchain setup

SYSROOT = ${NDK}/platforms/android-9/arch-arm

export PATH+=:${NDK}/toolchains/${TOOLCHAIN}-${TOOLCHAINVERS}/prebuilt/linux-x86/bin/

CPPFLAGS+= -I${SYSROOT}/usr/include
LDFLAGS = -L${SYSROOT}/usr/lib

CC= $(TOOLCHAIN)-gcc --sysroot=${SYSROOT}
LD= $(TOOLCHAIN)-gcc

### Build setup

BIN= pgdn
OBJS= $(SRCS:.c=.o)

all: $(BIN) $(BIN).zip

$(BIN): $(OBJS)

push: $(BIN)
	adb push $(BIN) /data/local/bin/$(BIN)
	adb shell chmod 755 /data/local/bin/$(BIN)

push-config:
	adb push dot.pgdn /sdcard/.pgdn

dist: $(BIN).zip

$(BIN).zip: $(BIN)
	rm -f $@
	zip $@ pgdn dot.pgdn

clean:
	rm -rf $(OBJS) $(BIN) $(BIN).zip
