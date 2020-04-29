CXX_FLAGS=-std=c++17 -Wall -O3 -march=native
RMI_PARAMETERS=rmi_data/books_L1_PARAMETERS rmi_data/fb_L1_PARAMETERS rmi_data/osm_L1_PARAMETERS rmi_data/wiki_L1_PARAMETERS

results.txt: benchmark execute.sh
	./download.sh
	./execute.sh

benchmark: main.cpp $(RMI_PARAMETERS) rmis/osm.cpp rmis/wiki.cpp rmis/books.cpp rmis/fb.cpp
	git submodule update --init --recursive
	g++ $(CXX_FLAGS) main.cpp rmis/fb.cpp rmis/wiki.cpp rmis/osm.cpp rmis/books.cpp -o benchmark

rmi_data:
	mkdir rmi_data

rmi_data/books_L1_PARAMETERS: rmi_data_compressed/books_L1_PARAMETERS.zst rmi_data
	zstd -fd $< -o $@

rmi_data/fb_L1_PARAMETERS: rmi_data_compressed/fb_L1_PARAMETERS.zst rmi_data
	zstd -fd $< -o $@

rmi_data/osm_L1_PARAMETERS: rmi_data_compressed/osm_L1_PARAMETERS.zst rmi_data
	zstd -fd $< -o $@

rmi_data/wiki_L1_PARAMETERS: rmi_data_compressed/wiki_L1_PARAMETERS.zst rmi_data
	zstd -fd $< -o $@



.PHONY: clean
clean:
	rm -rf rmi_data benchmark results.txt
