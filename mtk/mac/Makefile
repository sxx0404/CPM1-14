GCC=../toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
GCPP=../toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-g++
#GCC=gcc
#GCPP=g++
OBJ=$(patsubst %.c,%.o,$(wildcard *.c))
OUT=mac
CFLAGS=-Wall
LFLAGS=-Wall
LIBDIR=
LIBS=

.PHONY:clean

%.o:%.c
	$(GCC) -o $@ $(CFLAGS) -c $<
$(OUT):$(OBJ)
	$(GCC) -o $(OUT) $(LFLAGS) $^ $(LIBDIR) $(LIBS)
all:$(OUT)
	install -D $< ../down/home/$<
clean:
	rm -f *.o
	rm -f $(OUT)
rebuilt:clean all
