
CPP       = gcc

LIBS      =

CFLAGS    = -c -Wall -g

INCLUDE   = 

OFILES  = b_encode.o


%.o: %.c
	$(CPP) $(CFLAGS) $(INCLUDE) $<

%.o: %.cpp
	$(CPP) $(CFLAGS) $(INCLUDE) $<
	
b: $(OFILES)
	$(CPP) $(OFILES) $(LIBS) -o b
	
clean:
	- rm -f b
	- rm -f $(OFILES)
