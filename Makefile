CFLAGS := -std=c99 \
	-L/usr/local/lib \
	-L/usr/local/lib/x86_64-linux-gnu \
	-L/usr/lib/x86_64-linux-gnu \
	-I/usr/local/include \
	-I/usr/local/include/libdrm \
	-I/usr/include/libdrm

LDLIBS := -lgbm -ldrm

SRC := drm.c

all:
	gcc $(CFLAGS) $(SRC) $(LDLIBS) -o drm.out
	@echo OK!
