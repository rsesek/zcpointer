CXX := clang++
CXXFLAGS := -g -std=c++11

FILES := test.cc zcpointer.cc

all: test-zc test-tr

test: test-zc test-tr
	./test-zc
	./test-tr

test-zc: $(FILES) zcpointer.h
	$(CXX) $(CXXFLAGS) $(FILES) -o $@

test-tr: $(FILES) zcpointer.h
	$(CXX) -DZCPOINTER_TRACK_REFS=1 $(CXXFLAGS) $(FILES) -o $@

clean:
	rm -rf test-zc test-tr *.dSYM
