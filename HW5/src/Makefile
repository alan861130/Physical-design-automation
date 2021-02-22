CXX := g++
SOURSE := main.cpp
OUTPUT := main.o
TARGET := ../bin/hw5
version := -std=c++11

all: $(TARGET)

$(TARGET): $(SOURSE)
	$(CXX) $(version) -o $(TARGET) $(SOURSE)


$(OUTPUT): $(SOURSE)
	$(CXX) $(version) -c $(SOURSE)


clean:
	rm -rf $(TARGET) 
