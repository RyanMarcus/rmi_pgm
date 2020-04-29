CXX_FLAGS=-std=c++17 -Wall -O3 -march=native -ffast-math

benchmark: main.cpp rmis/osm.cpp rmis/wiki.cpp rmis/books.cpp rmis/fb.cpp
	git submodule update --init --recursive
	g++ $(CXX_FLAGS) main.cpp rmis/fb.cpp rmis/wiki.cpp rmis/osm.cpp rmis/books.cpp -o benchmark

results.txt: benchmark execute.sh
	./download.sh
	./execute.sh

.PHONY: clean
clean:
	rm -f benchmark results.txt
