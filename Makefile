CC = aarch64-linux-gnu-g++
CFLAGS = -std=c++11 -O3 -march=armv8-a+sve
GEM5_FLAGS = -I/home/user/xbyak_aarch64_handson/my-program/gem5/include -L/home/user/xbyak_aarch64_handson/my-program/gem5/util/m5/build/arm64/out -lm5

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
