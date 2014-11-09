
CC        = $(CROSS_COMPILE)g++
STRIP     = $(CROSS_COMPILE)strip

TARGET    = ctw

HASH_SRCS = hashcodes/crypto_hash.cpp hashcodes/sha1.cpp hashcodes/md5.cpp
HTTP_SRCS = http/httpclient.cpp http/httpcurl.cpp
AUTH_SRCS = oauth/base64.cpp oauth/oauth.cpp
TWCR_SRCS = twitter/twitter_client.cpp
UI_SRCS   = ui/simple_ui.cpp ui/simple_terminal.cpp
SRCS	  = main.cpp
OBJS	  = $(HASH_SRCS:%.cpp=%.o) $(HTTP_SRCS:%.cpp=%.o) $(AUTH_SRCS:%.cpp=%.o) $(TWCR_SRCS:%.cpp=%.o) $(UI_SRCS:%.cpp=%.o) $(SRCS:%.cpp=%.o)
DEPS	  = $(HASH_SRCS:%.cpp=%.d) $(HTTP_SRCS:%.cpp=%.d) $(AUTH_SRCS:%.cpp=%.d) $(TWCR_SRCS:%.cpp=%.d) $(UI_SRCS:%.cpp=%.d) $(SRCS:%.cpp=%.d)
KEYS	  = keys/apikeys.hpp
INCLUDE_DIRS = ./hashcodes ./http ./oauth ./twitter ./ui ./include .

CFLAGS    = -Wall -O2 -fPIC -shared $(addprefix -I,$(INCLUDE_DIRS))
LDFLAGS   = -lcurl

ifndef CONSUMER_KEY
$(error CONSUMER_KEY can not find from enviroment! you must define CONSUMER_KEY)
endif
ifndef CONSUMER_SECRET
$(error CONSUMER_SECRET can not find from enviroment! you must define CONSUMER_SECRET)
endif


all: $(KEYS) $(TARGET)

-include $(DEPS)

clean: 
	rm -f $(OBJS) $(DEPS) $(TARGET) *~ *.scc *.vcproj *.vspscc $(KEYS)

$(KEYS):
	-@ ./write_apikeys.sh

$(TARGET): $(OBJS) 
	$(CC) $(CFLAGS) $(DEBUGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@


.cpp.o:
	$(CC) -c -MMD -MP -o $@ $(CFLAGS) $(DEBUGS) $<

