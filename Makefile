CC = aarch64-linux-gnu-g++-4.8
CFLAGS = -std=c++11 -O3 -march=armv8-a
GEM5_FLAGS = -I/home/wangzicong/Projects/gem5-X/include -L/home/wangzicong/Projects/gem5-X/util/m5 -lm5

all: float double static

float: spmv_native_float spmv_gem5_float

double: spmv_native_double spmv_gem5_double

static: spmv_gem5_float_static spmv_gem5_double_static

spmv_native_float: spmv.cpp
	 $(CC) spmv.cpp $(CFLAGS) -o $@ -DSP -DNATIVE -static

spmv_gem5_float: spmv.cpp
	 $(CC) spmv.cpp $(CFLAGS) $(GEM5_FLAGS) -o $@ -DSP

spmv_native_double: spmv.cpp
	 $(CC) spmv.cpp $(CFLAGS) -o $@ -DNATIVE -static

spmv_gem5_double: spmv.cpp
	 $(CC) spmv.cpp $(CFLAGS) $(GEM5_FLAGS) -o $@

spmv_gem5_float_static: spmv.cpp
	 $(CC) spmv.cpp $(CFLAGS) $(GEM5_FLAGS) -o $@ -DSP -static

spmv_gem5_double_static: spmv.cpp
	 $(CC) spmv.cpp $(CFLAGS) $(GEM5_FLAGS) -o $@ -static

.PHONY: clean

clean:
	rm spmv_*_*
