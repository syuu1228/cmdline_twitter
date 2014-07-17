
CC        = $(CROSS_COMPILE)g++
STRIP     = $(CROSS_COMPILE)strip
AR        = $(CROSS_COMPILE)ar
RANLIB    = $(CROSS_COMPILE)ranlib

CFLAGS    = -Wall -O2
LDFLAGS   = -lcurl

TARGET    = httplib.a

OBJS	  = httpclient.o httpcurl.o

all: $(TARGET)

clean: 
	rm -f *.o $(TARGET) *~ *.scc *.vcproj *.vspscc


$(TARGET): $(OBJS)
	$(AR) rvs  -o $@ $(OBJS)

.cpp.o:
	$(CC) -c $(CFLAGS) $(DEBUGS) $<

httpclient.o: httpclient.cpp httpclient.hpp
httpcurl.o: httpcurl.cpp httpcurl.hpp httpclient.hpp


