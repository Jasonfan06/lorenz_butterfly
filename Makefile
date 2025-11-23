CXX = clang++
CXXFLAGS = -std=c++17 -O3 -march=native -Wall
INCLUDES = -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib
LIBS = -lglfw -framework OpenGL -framework Cocoa -framework IOKit

TARGET = lorenz_butterfly
SOURCE = lorenz_butterfly.cpp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(LDFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)
	@echo ""
	@echo "✓ Build successful!"
	@echo "Run with: ./$(TARGET)"

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

