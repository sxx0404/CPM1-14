GCC=../../toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
GCPP=../../toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-g++
#GCC=gcc
#GCPP=g++
OBJ=$(patsubst %.c,%.o,$(wildcard *.c))

BVER=1
SVER=0.0
FVER=$(BVER).$(SVER)
NAME=jxjcam
OUT=lib$(NAME).so.$(FVER)

.PHONY:clean

%.o:%.c
	$(GCC) -c -fPIC -Wall $<
$(OUT):$(OBJ)
	$(GCC) -o $(OUT) -Wall -shared -Wl,-soname,lib$(NAME).so.$(BVER) $(OBJ) -L./ -ljcusdk
all:$(OUT)
	ln -sf libev.so.4.0.0 libev.so
	ln -sf $(OUT) lib$(NAME).so
	ln -sf $(OUT) lib$(NAME).so.$(BVER)
	install -D $(OUT) ../../down/lib/$(OUT)
	ln -sf $(OUT) ../../down/lib/lib$(NAME).so.$(BVER)
	ln -sf lib$(NAME).so.$(BVER) ../../down/lib/lib$(NAME).so
clean:
	rm -f *.o
	rm -f lib$(NAME).so.$(BVER)
	rm -f $(OUT)
rebuilt:clean all
