#include <iostream>
#include <cfloat>
#include <cmath>
#include <png++/png.hpp>
#include "AdecError.h"

using namespace png;
using namespace std;

double from8(byte x)
{
    const double k = 1 / 255.0;
    return x * k;
}

double from4(byte x)
{
    const double k = 1 / 15.0;
    return x * k;
}

template <typename T>
T clamp(image<T> im, int x, int y)
{
    x = max(0, min((int)im.get_width() - 1, x));
    y = max(0, min((int)im.get_height() - 1, y));
    return im.get_pixel(x, y);
}

rgba_pixel pixel16(int r4, int g4, int b4, int a4)
{
    return rgba_pixel(r4 * 17, g4 * 17, b4 * 17, a4 * 17);
}

double minimize(AdecError& e, int& f, int& b)
{
    e.C_f3 = from4(f);
    e.C_b3 = from4(b);
    double error = e.calcResult();
    while(true)
    {
        int f2 = f;
        int b2 = b;
        if (f > 0)
        {
            e.C_f3 = from4(f - 1);
            double e2 = e.calcResult();
            if (e2 < error)
            {
                error = e2;
                f2 = f - 1;
            }
        }
        if (f < 15)
        {
            e.C_f3 = from4(f + 1);
            double e2 = e.calcResult();
            if (e2 < error)
            {
                error = e2;
                f2 = f + 1;
            }
        }
        e.C_f3 = from4(f);
        if (b > 0)
        {
            e.C_b3 = from4(b - 1);
            double e2 = e.calcResult();
            if (e2 < error)
            {
                error = e2;
                f2 = f;
                b2 = b - 1;
            }
        }
        if (b < 15)
        {
            e.C_b3 = from4(b + 1);
            double e2 = e.calcResult();
            if (e2 < error)
            {
                error = e2;
                f2 = f;
                b2 = b + 1;
            }
        }
        if (f2 == f && b2 == b)
        {
            return error;
        }
        f = f2;
        b = b2;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << "[src.png] [front.png] [back.png]" << endl;
        return EXIT_FAILURE;
    }
    try {
        image<rgba_pixel> src(argv[1]);
        size_t w = src.get_width();
        size_t h = src.get_height();
        image<rgba_pixel> front(w, h);
        image<rgba_pixel> back(w, h);
        AdecError aer, aeg, aeb;
        for (size_t x = 0; x < w; ++x)
        for (size_t y = 0; y < h; ++y)
        {
            rgba_pixel s0 = clamp(src, x - 1, y - 1);
            rgba_pixel s1 = clamp(src, x, y - 1);
            rgba_pixel s2 = clamp(src, x - 1, y);
            rgba_pixel s3 = clamp(src, x, y);

            rgba_pixel f0 = clamp(front, x - 1, y - 1);
            rgba_pixel f1 = clamp(front, x, y - 1);
            rgba_pixel f2 = clamp(front, x - 1, y);

            rgba_pixel b0 = clamp(back, x - 1, y - 1);
            rgba_pixel b1 = clamp(back, x, y - 1);
            rgba_pixel b2 = clamp(back, x - 1, y);

            aer.A_s0 = from8(s0.alpha);
            aer.A_s1 = from8(s1.alpha);
            aer.A_s2 = from8(s2.alpha);
            aer.A_s3 = from8(s3.alpha);
            aer.A_f0 = from8(f0.alpha);
            aer.A_f1 = from8(f1.alpha);
            aer.A_f2 = from8(f2.alpha);
            aer.A_b0 = from8(b0.alpha);
            aer.A_b1 = from8(b1.alpha);
            aer.A_b2 = from8(b2.alpha);
            aer.calcTemp0();

            aer.C_s0 = from8(s0.red);
            aer.C_s1 = from8(s1.red);
            aer.C_s2 = from8(s2.red);
            aer.C_s3 = from8(s3.red);
            aer.C_f0 = from8(f0.red);
            aer.C_f1 = from8(f1.red);
            aer.C_f2 = from8(f2.red);
            aer.C_b0 = from8(b0.red);
            aer.C_b1 = from8(b1.red);
            aer.C_b2 = from8(b2.red);
            aer.calcTemp1();

            aeg.copyTemp0(aer);
            aeg.C_s0 = from8(s0.green);
            aeg.C_s1 = from8(s1.green);
            aeg.C_s2 = from8(s2.green);
            aeg.C_s3 = from8(s3.green);
            aeg.C_f0 = from8(f0.green);
            aeg.C_f1 = from8(f1.green);
            aeg.C_f2 = from8(f2.green);
            aeg.C_b0 = from8(b0.green);
            aeg.C_b1 = from8(b1.green);
            aeg.C_b2 = from8(b2.green);
            aeg.calcTemp1();

            aeb.copyTemp0(aer);
            aeb.C_s0 = from8(s0.blue);
            aeb.C_s1 = from8(s1.blue);
            aeb.C_s2 = from8(s2.blue);
            aeb.C_s3 = from8(s3.blue);
            aeb.C_f0 = from8(f0.blue);
            aeb.C_f1 = from8(f1.blue);
            aeb.C_f2 = from8(f2.blue);
            aeb.C_b0 = from8(b0.blue);
            aeb.C_b1 = from8(b1.blue);
            aeb.C_b2 = from8(b2.blue);
            aeb.calcTemp1();

            double minError = DBL_MAX;
            int min_R_f3_4, min_R_b3_4;
            int min_G_f3_4, min_G_b3_4;
            int min_B_f3_4, min_B_b3_4;
            int min_A_f3_4, min_A_b3_4;

            double A_s3 = from8(s3.alpha);
            for (int A_f3_4 = 0; A_f3_4 < 16; ++A_f3_4)
            for (int A_b3_4 = A_f3_4; A_b3_4 < 16; ++A_b3_4)
            {
                double A_f3 = from4(A_f3_4);
                double A_b3 = from4(A_b3_4);
                double error = 0.0;
                double A_fb3 = 1.0 - (1.0 - A_f3) * (1.0 - A_b3);
                const double thresh = 1 / 15.0;
                if (fabs(A_s3 - A_fb3) <= thresh)
                {
                    int R_f3_4 = s3.red >> 4;
                    int R_b3_4 = R_f3_4;
                    int G_f3_4 = s3.green >> 4;
                    int G_b3_4 = G_f3_4;
                    int B_f3_4 = s3.blue >> 4;
                    int B_b3_4 = B_f3_4;

                    aer.A_f3 = A_f3;
                    aer.A_b3 = A_b3;
                    aer.calcTemp2();
                    error += minimize(aer, R_f3_4, R_b3_4);

                    aeg.A_f3 = A_f3;
                    aeg.A_b3 = A_b3;
                    aeg.calcTemp2();
                    error += minimize(aeg, G_f3_4, B_b3_4);

                    aeb.A_f3 = A_f3;
                    aeb.A_b3 = A_b3;
                    aeb.calcTemp2();
                    error += minimize(aeb, B_f3_4, B_b3_4);

                    if (error < minError)
                    {
                        min_R_f3_4 = R_f3_4;
                        min_R_b3_4 = R_b3_4;
                        min_G_f3_4 = G_f3_4;
                        min_G_b3_4 = G_b3_4;
                        min_B_f3_4 = B_f3_4;
                        min_B_b3_4 = B_b3_4;
                        min_A_f3_4 = A_f3_4;
                        min_A_b3_4 = A_b3_4;
                    }
                }
            }
            front.set_pixel(x, y, pixel16(min_R_f3_4, min_G_f3_4, min_B_f3_4, min_A_f3_4));
            back.set_pixel(x, y, pixel16(min_R_b3_4, min_G_b3_4, min_B_b3_4, min_A_b3_4));
        }
        front.write(argv[2]);
        back.write(argv[3]);
    } catch (png::error& error) {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }
    return 0;
}
