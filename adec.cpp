#include <iostream>
#include <limits>
#include <png++/png.hpp>
#include <cln/integer.h>
#include "AdecError.h"

using namespace png;
using namespace std;
using namespace cln;

// A pseudo wrap mode for textures packed with transparent margins.
rgba_pixel packed(image<rgba_pixel> im, int x, int y)
{
    if (x < 0 || y < 0 || x >= im.get_width() || y >= im.get_height())
        return rgba_pixel(0, 0, 0, 0);
    else
        return im.get_pixel(x, y);
}

// Convert 16 bpp RGBA to 32 bpp. (PNG doesn't support 16 bpp!)
rgba_pixel pixel16(int r4, int g4, int b4, int a4)
{
    return rgba_pixel(r4 * 17, g4 * 17, b4 * 17, a4 * 17);
}

// Minimize error of the component.
long bruteforce(AdecError& e, int& f, int& b)
{
    long minError = LLONG_MAX;
    for (e.C_f3 = 0; e.C_f3 < 16; ++e.C_f3)
    for (e.C_b3 = 0; e.C_b3 < 16; ++e.C_b3)
    {
        long error = e.calcResult();
        if (error < minError)
        {
            minError = error;
            f = e.C_f3;
            b = e.C_b3;
        }
    }
    return minError;
}

// Alpha decomposition by analytical integration.
void alphaDecompose(const image<rgba_pixel> & src, image<rgba_pixel> & front, image<rgba_pixel> & back)
{
    size_t w = src.get_width();
    size_t h = src.get_height();
    AdecError aer, aeg, aeb;
    for (size_t y = 0; y < h; ++y)
    {
        cerr << "\r" << w * y << "/" << w * h << flush;
        for (size_t x = 0; x < w; ++x)
        {
            rgba_pixel s0 = packed(src, x - 1, y - 1);
            rgba_pixel s1 = packed(src, x, y - 1);
            rgba_pixel s2 = packed(src, x - 1, y);
            rgba_pixel s3 = packed(src, x, y);

            rgba_pixel f0 = packed(front, x - 1, y - 1);
            rgba_pixel f1 = packed(front, x, y - 1);
            rgba_pixel f2 = packed(front, x - 1, y);

            rgba_pixel b0 = packed(back, x - 1, y - 1);
            rgba_pixel b1 = packed(back, x, y - 1);
            rgba_pixel b2 = packed(back, x - 1, y);

            aer.A_s0 = s0.alpha;
            aer.A_s1 = s1.alpha;
            aer.A_s2 = s2.alpha;
            aer.A_s3 = s3.alpha;
            aer.A_f0 = f0.alpha >> 4;
            aer.A_f1 = f1.alpha >> 4;
            aer.A_f2 = f2.alpha >> 4;
            aer.A_b0 = b0.alpha >> 4;
            aer.A_b1 = b1.alpha >> 4;
            aer.A_b2 = b2.alpha >> 4;
            aer.calcTemp0();

            aer.C_s0 = s0.red;
            aer.C_s1 = s1.red;
            aer.C_s2 = s2.red;
            aer.C_s3 = s3.red;
            aer.C_f0 = f0.red >> 4;
            aer.C_f1 = f1.red >> 4;
            aer.C_f2 = f2.red >> 4;
            aer.C_b0 = b0.red >> 4;
            aer.C_b1 = b1.red >> 4;
            aer.C_b2 = b2.red >> 4;
            aer.calcTemp1();

            aeg.copyTemp0(aer);
            aeg.C_s0 = s0.green;
            aeg.C_s1 = s1.green;
            aeg.C_s2 = s2.green;
            aeg.C_s3 = s3.green;
            aeg.C_f0 = f0.green >> 4;
            aeg.C_f1 = f1.green >> 4;
            aeg.C_f2 = f2.green >> 4;
            aeg.C_b0 = b0.green >> 4;
            aeg.C_b1 = b1.green >> 4;
            aeg.C_b2 = b2.green >> 4;
            aeg.calcTemp1();

            aeb.copyTemp0(aer);
            aeb.C_s0 = s0.blue;
            aeb.C_s1 = s1.blue;
            aeb.C_s2 = s2.blue;
            aeb.C_s3 = s3.blue;
            aeb.C_f0 = f0.blue >> 4;
            aeb.C_f1 = f1.blue >> 4;
            aeb.C_f2 = f2.blue >> 4;
            aeb.C_b0 = b0.blue >> 4;
            aeb.C_b1 = b1.blue >> 4;
            aeb.C_b2 = b2.blue >> 4;
            aeb.calcTemp1();

            cl_I minError = cl_I(numeric_limits<long>::max()) * 3;
            int min_R_f3, min_R_b3;
            int min_G_f3, min_G_b3;
            int min_B_f3, min_B_b3;
            int min_A_f3, min_A_b3;

            for (int A_b3 = 0; A_b3 < 16; ++A_b3)
            for (int A_f3 = 0; A_f3 < 16; ++A_f3)
            {
                cl_I error = 0;
                int R_f3 = s3.red >> 4;
                int R_b3 = R_f3;
                int G_f3 = s3.green >> 4;
                int G_b3 = G_f3;
                int B_f3 = s3.blue >> 4;
                int B_b3 = B_f3;

                aer.A_f3 = A_f3;
                aer.A_b3 = A_b3;
                aer.calcTemp2();
                error += bruteforce(aer, R_f3, R_b3);

                aeg.A_f3 = A_f3;
                aeg.A_b3 = A_b3;
                aeg.calcTemp2();
                error += bruteforce(aeg, G_f3, G_b3);

                aeb.A_f3 = A_f3;
                aeb.A_b3 = A_b3;
                aeb.calcTemp2();
                error += bruteforce(aeb, B_f3, B_b3);

                if (error < minError)
                {
                    minError = error;
                    min_R_f3 = R_f3;
                    min_R_b3 = R_b3;
                    min_G_f3 = G_f3;
                    min_G_b3 = G_b3;
                    min_B_f3 = B_f3;
                    min_B_b3 = B_b3;
                    min_A_f3 = A_f3;
                    min_A_b3 = A_b3;
                }
            }
            front.set_pixel(x, y, pixel16(min_R_f3, min_G_f3, min_B_f3, min_A_f3));
            back.set_pixel(x, y, pixel16(min_R_b3, min_G_b3, min_B_b3, min_A_b3));
        }
    }
    cerr << endl;
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        cerr << "usage: " << argv[0] << "[src.png] [front.png] [back.png]" << endl;
        return EXIT_FAILURE;
    }
    image<rgba_pixel> src(argv[1]);
    size_t w = src.get_width();
    size_t h = src.get_height();
    image<rgba_pixel> front(w, h);
    image<rgba_pixel> back(w, h);
    alphaDecompose(src, front, back);
    front.write(argv[2]);
    back.write(argv[3]);
    return EXIT_SUCCESS;
}
