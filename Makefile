CXX = g++
CXXFLAGS = -O2 -std=c++17
TARGET = eventfd_benchmark
SRC = eventfd_benchmark.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -f $(TARGET)
