TARGET=ace_console
ifeq ($(arch),arm64)
CC=aarch64-linux-gnu-g++
STRIP=aarch64-linux-gnu-strip
else
CC=g++
STRIP=strip
endif
product?=nologo
CFLAGS=-D$(product)
CFLAGS2=-D$(product)
ifeq ($(arch),arm64)
CFLAGS+=-Wall -Wno-write-strings -O -g -I /opt/hy-arm64/toolchain/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/include/ -I../../ace_include -DARCH_ARM64
CFLAGS2+=-Wall -O -g -L /opt/hy-arm64/toolchain/gcc-linaro-5.3.1-2016.05-x86_64_aarch64-linux-gnu/lib/ -lpthread -lncurses -lssl -lcrypto
else
CFLAGS+=-Wall -Wno-write-strings -O -g -I/usr/include -I/usr/include/ncurses -I../../ace_include
CFLAGS2+=-Wall -O -g -L/usr/lib -lpthread -lncurses -ltinfo -lssl -lcrypto
endif

ifeq ($(firewall),1)
CFLAGS+=-DCONFIG_HYTF_FW
endif
CFLAGS+=$(hylab_comm_macro)
SRC=$(wildcard *.cpp)
OBJS=$(patsubst %.cpp,%.o,$(wildcard *.cpp))

$(TARGET):$(OBJS)
	$(CC) $(OBJS) $(CFLAGS2) -o $@
	$(STRIP) $@

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o ace_console 

