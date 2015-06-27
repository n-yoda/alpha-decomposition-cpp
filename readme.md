# alpha-decomposition-cpp
C++ implementation of alpha decomposition.

## Usage
```
make adec
./adec input.png output_front.png output_back.png
```
(Current makefile supports MacOSX only.)

If you have ImageMagick, try the following command for verification.
```
convert output_back.png output_front.png -composite similar_to_input.png
```

## Dependencies
* GiNaC
* libpng
