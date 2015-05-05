generate_AdecError: generate_AdecError.cpp
	g++ -std=c++11 -O2 -lginac -lcln $< -o $@

AdecError.h: generate_AdecError
	./$< > $@

clean:
	rm -f generate_AdecError
