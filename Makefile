CXX = g++
CXXFLAGS = -Iheaders

all:
	$(CXX) $(CXXFLAGS)  cpu.cpp memory.cpp dma.cpp dma_channels.cpp -o test
	./test

clean:
	rm -f test
