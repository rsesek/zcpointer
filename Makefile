CXX := clang++
CXXFLAGS := -g -std=c++11

ifeq ($(OPTIMIZED),1)
	CXXFLAGS += -O2
endif

FILES := test.cc zcpointer.cc

DEPS := $(FILES) zcpointer.h Makefile

all: test-zc test-tr

test: test-zc test-tr
	./test-zc
	./test-tr

test-zc: $(DEPS)
	$(CXX) $(CXXFLAGS) $(FILES) -o $@

test-tr: $(DEPS)
	$(CXX) -DZCPOINTER_TRACK_REFS=1 $(CXXFLAGS) $(FILES) -o $@

clean:
	rm -rf test-zc test-tr *.dSYM
