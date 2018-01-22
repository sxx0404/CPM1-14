
CURPATH = $(shell pwd)
#CROSS=/a10/Cusdk/src/toolchain/toolchain-mipsel_24kec+dsp_gcc-4.8-linaro_uClibc-0.9.33.2/bin/mipsel-openwrt-linux-

CROSS?=
SRC_ = $(shell ls ../src/*.c)

OS?=
SRC = $(SRC_)
OBJ=$(SRC:.c=.o)

TARGET= demo
CC=$(CROSS)gcc
LD=$(CROSS)ld
AR=$(CROSS)ar
CFLAGS=-I $(CURPATH)/../inc/ 
LDFLAGS= -L.. -lpthread -lrt -lm -ldl -lev


all: $(TARGET) ../$(TARGET)so

../$(TARGET)so: $(OBJ)
		$(CC) -ljcusdk $(LDFLAGS) -o ../$(TARGET)so $^ 

$(TARGET): $(OBJ)
#         @rm -f lib$(TARGET).a
		$(CC) $(LDFLAGS) -o $(TARGET) $^ ../libjcusdk.a
#		$(CC) $(LDFLAGS) -o $(TARGET) $^ $(LIBS)
#         @$(AR) crv  lib$(TARGET).a $(TARGET).o
#         @rm -f $(TARGET).o
#         @cp inc/* ../inc
#         @cp lib$(TARGET).a ../lib
#         @cp $(DEPEND_PATH)/inc/* ../inc

OBD=$(OBJ:.o=.d)
$(OBD):%.d: %.c ../inc/*.h
	@set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$ 2>/dev/null; \
	sed 's,\(.*:\)\(.*\),../src/\1 $@ \2,g' < $@.$$$$ > $@; \
	echo -e "\t${CC} -c $< ${CFLAGS} -o ${@:.d=.o}">>$@;\
	rm -f $@.$$$$

ifeq ($(MAKECMDGOALS), )
-include $(OBD)
endif 

# INCS_ = $(shell ls $(DEPEND_PATH)/inc/  && ls inc/)
# INCS  = $(addprefix ../inc/, $(INCS_))
# 
.PHONY: clean 
# depend:
#         make -C $(DEPEND_PATH) $(EX_MAKEFLAG) \
#                 || echo -e '\e[31m'!!!!!!!ERROR!!!!!!! when excute `pwd`/$(DEPEND_PATH)/Makefile '\e[m'
clean:
	@rm -f $(TARGET) ../$(TARGET)so  $(OBJ) 
#         @rm -f ../lib/lib$(TARGET).a 
#         @rm -f $(INCS) #*
#         $(excute_make_clean_depend)
# cleand:
#         make clean -C $(DEPEND_PATH) $(EX_MAKEFLAG)
# 
#$(if $? == 0, echo "yes", echo "no")
# define make_subdir
# @for subdir in $(SUBDIRS) ; do \
#         ( cd $$subdir && make $1) \
#         done;
# endef


