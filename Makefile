generate_AdecError: generate_AdecError.cpp
	g++ -std=c++11 -O2 -lginac -lcln $< -o $@

AdecError.h: generate_AdecError
	./$< > $@

adec: adec.cpp AdecError.h
	g++ `libpng-config --cppflags --ldflags` -O2 $< -o $@

clean:
	rm -f generate_AdecError
