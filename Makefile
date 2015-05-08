generate_AdecError: generate_AdecError.cpp
	g++ -std=c++11 -O2 -lginac -lcln -Wall $< -o $@

AdecError.h: generate_AdecError
	./$< > $@

adec: adec.cpp AdecError.h
	g++ `libpng-config --cppflags --ldflags` -lcln -Wall -O3 $< -o $@

clean:
	rm -f generate_AdecError
