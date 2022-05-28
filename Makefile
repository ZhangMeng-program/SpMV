spmv: spmv.cpp
	aarch64-linux-gnu-g++ spmv.cpp -std=c++11 -o spmv -I/home/wangzicong/Projects/gem5-X/include -L/home/wangzicong/Projects/gem5-X/util/m5 -lm5 -O3
clean:
	rm spmv
