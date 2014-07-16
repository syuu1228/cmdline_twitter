
CC        = $(CROSS_COMPILE)g++
STRIP     = $(CROSS_COMPILE)strip
AR        = $(CROSS_COMPILE)ar
RANLIB    = $(CROSS_COMPILE)ranlib

CFLAGS    = -Wall
LDFLAGS   =  

TARGET    = hashlib.a

OBJS	  = crypto_hash.o sha1.o md5.o

all: $(TARGET)

clean: 
	rm -f *.o $(TARGET) *~ *.scc *.vcproj *.vspscc


$(TARGET): $(OBJS)
	$(AR) rvs  -o $@ $(OBJS)


.cpp.o:
	$(CC) -c $(CFLAGS) $(DEBUGS) $<

crypto_hash.o: crypto_hash.cpp crypto_hash.hpp
sha1.o: sha1.cpp sha1.hpp crypto_hash.hpp byteorder.hpp
md5.o: md5.cpp md5.hpp crypto_hash.hpp byteorder.hpp
