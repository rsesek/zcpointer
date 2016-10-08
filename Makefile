CXX := clang++
CXXFLAGS := -g -std=c++11

all: test-zc test-tr

test-zc: test.cc
	$(CXX) $(CXXFLAGS) $< -o $@

test-tr: test.cc
	$(CXX) -DZCPOINTER_TRACK_REFS=1 $(CXXFLAGS) $< -o $@

clean:
	rm -rf test-zc test-tr *.dSYM
