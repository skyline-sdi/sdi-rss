CXX = c++
CXXFLAGS = -O3 -m64 -std=c++11

SOURCE = sdis-*.cpp timer.cpp

all: rss rssi

bin:
	mkdir -p bin

rss: rss-count rss-time

rss-count: bin
	$(CXX) $(CXXFLAGS) -o bin/$@ $@.cpp $(SOURCE)

rss-time: bin
	$(CXX) $(CXXFLAGS) -o bin/$@ $@.cpp $(SOURCE) -DWITH_TIME_WINDOW

rssi: rssi-count rssi-time

rssi-count: bin
	$(CXX) $(CXXFLAGS) -o bin/$@ $@.cpp $(SOURCE)

rssi-time: bin
	$(CXX) $(CXXFLAGS) -o bin/$@ $@.cpp $(SOURCE) -DWITH_TIME_WINDOW

clean:
	rm -rf bin
