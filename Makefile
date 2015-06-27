adec: adec.cpp AdecError.h
	g++ `libpng-config --cppflags --ldflags` -lcln -Wall -O3 $< -o $@

generate_AdecError: generate_AdecError.cpp
	g++ -std=c++11 -O2 -lginac -lcln -Wall $< -o $@

AdecError.h: generate_AdecError
	./$< > $@

clean:
	rm -f adec generate_AdecError AdecError.h
