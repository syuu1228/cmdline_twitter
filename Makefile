
CC        = $(CROSS_COMPILE)g++
STRIP     = $(CROSS_COMPILE)strip

CFLAGS    = -Wall -O2
LDFLAGS   = -lcurl

ifeq ($(MSYSTEM),MINGW32)
TARGET    = ctw.exe
else
TARGET    = ctw
endif

OBJS	  = base64.o oauth.o twitter_client.o main.o keys/apikeys.hpp
ARLIBS    = http/httplib.a hashcodes/hashlib.a

all: hashlib httplib $(TARGET)

clean: 
	rm -f *.o $(TARGET) *~ *.scc *.vcproj *.vspscc  keys/apikeys.hpp
	-@ cd http;	make clean
	-@ cd hashcodes;	make clean

keys/apikeys.hpp:
	-@ ./write_apikeys.sh


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(DEBUGS) -o $@ $(OBJS) $(ARLIBS) $(LDFLAGS)
	$(STRIP) $@

httplib:
	-@ cd http;	make 

hashlib:
	-@ cd hashcodes;	make 

.cpp.o:
	$(CC) -c $(CFLAGS) $(DEBUGS) $<

base64.o: base64.cpp base64.hpp 
oauth.o: oauth.cpp oauth.hpp $(ARLIBS)
twitter_client.o: twitter_client.cpp twitter_client.hpp oauth.hpp picojson.h $(ARLIBS)
main.o: main.cpp $(ARLIBS) twitter_client.hpp keys/apikeys.hpp

