GCC=../toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-gcc
GCPP=../toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-g++
CSRC=$(wildcard *.c nm/*.c mgt/*.c lc/*.c frsdb/*.c up/*.c sd/*c)
CPPSRC=$(wildcard *.cpp nm/*.cpp mgt/*.cpp lc/*.cpp frsdb/*.cpp up/*.cpp sd/*.cpp)
COBJ=$(patsubst %.c,%.o,$(CSRC))
CPPOBJ=$(patsubst %.cpp,%.o,$(CPPSRC))
#OBJ=$(COBJ) $(CPPOBJ) nm/gprs/GprsAPI.o nm/GPRSDrv.o nm/uart/uart.o nm/socket/socket.o nm/ping.o cam/cam.o mgt/mgt.o lc/RfApp.o lc/Rfdrv.o lc/Rflink.o lc/Rftransmit.o
OBJ=$(COBJ) $(CPPOBJ)
OUT=bn
CFLAGS=-pthread -Wall
LFLAGS=-pthread -Wall
LIBDIR=-L../libs/jxjcam -L../libs/common -L../libs/log -L../libs/gpio
LIBS=-lev -lcommon -lrt -lm -llog -ldl -lgpio

%.o:%.c
	$(GCC) -o $@ $(CFLAGS) -c $<
%.o:%.cpp
	$(GCPP) -o $@ $(CFLAGS) -c $<

clean:
	rm -f $(OUT) $(OBJ)
$(OUT):$(OBJ)
	$(GCPP) -o $(OUT) $(LFLAGS) $^ $(LIBDIR) $(LIBS)
all: $(OUT)
	install -D $< ../down/home/$<
rebuilt:clean all
