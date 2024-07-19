CMAKE = cmake
BUILD_DIR = build
CMAKE_FLAGS = -GNinja -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
	-DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang

all: build

build:
	mkdir -p $(BUILD_DIR)
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)
	cmake --build $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all build clean

