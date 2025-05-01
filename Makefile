CXX = g++
CXXFLAGS = -fPIC -shared -std=c++11
INCLUDES = -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LIBS = -lpjsua2 -lpjsua -lpjsip-ua -lpjsip -lpjmedia-codec -lpjmedia -lpjlib-util -lpj -lssl -lcrypto -lsrtp -lpthread

SRC = pjsua2_manager.cpp
OUT = libpjsua2_manager.so

all: $(OUT)

$(OUT): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(OUT)