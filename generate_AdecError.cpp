#include <iostream>
#include <fstream>
#include <sstream>
#include <ginac/ginac.h>
#include <ginac/archive.h>

using namespace std;
using namespace GiNaC;

template <typename TRGB, typename TA>
struct Color
{
public:
    TRGB rgb;
    TA alpha;
    Color (const Color& c) : rgb(c.rgb), alpha(c.alpha) {}
    Color () : rgb(), alpha() {}
    Color (const TRGB& r, const TA& a) : rgb(r), alpha(a) {}
};

template<typename TRGB, typename TA>
TA iseabc(const Color<TRGB, TA>& a, const Color<TRGB, TA>& b)
{
    TRGB aRgbError = a.alpha * a.rgb - b.alpha * b.rgb;
    TA aError = a.alpha - b.alpha;
    return aRgbError * (aRgbError - aError);
}

template<typename TRGB, typename TA>
Color<TRGB, TA> linear(
    const Color<TRGB, TA>& c0, const Color<TRGB, TA>& c1, const TA& t)
{
    return Color<TRGB, TA>(
        (1 - t) * c0.rgb + t * c1.rgb,
        (1 - t) * c0.alpha + t * c1.alpha);
}

template<typename TRGB, typename TA>
Color<TRGB, TA> bilinear(
    const Color<TRGB, TA> &c0, const Color<TRGB, TA> &c1,
    const Color<TRGB, TA> &c2, const Color<TRGB, TA> &c3,
    const TA& u, const TA& v)
{
    Color<TRGB, TA> c01 = linear(c0, c1, u);
    Color<TRGB, TA> c23 = linear(c2, c3, u);
    return linear(c01, c23, v);
}

template<typename TRGB, typename TA>
Color<TRGB, TA> alphaBlend(
    const Color<TRGB, TA> &f, const Color<TRGB, TA> &b)
{
    Color<TRGB, TA> blend;
    blend.alpha = 1 - (1 - f.alpha) * (1 - b.alpha);
    TRGB aRgb = f.alpha * f.rgb + (1 - f.alpha) * b.alpha * b.rgb;
    blend.rgb = aRgb / blend.alpha;
    return blend;
}

struct ExColor1 : public Color<ex, ex>
{
public:
    symbol rgbSymbol;
    symbol alphaSymbol;
    ExColor1(const string& name) :
        rgbSymbol(string("C_").append(name)),
        alphaSymbol(string("A_").append(name)),
        Color() {
            rgb = ex(rgbSymbol);
            alpha = ex(alphaSymbol);
        }
};

ex partial(const ex& src, const lst& applyArgs, const string& tempName, vector<pair<symbol, ex>>& temps)
{
    lst argsSubs;
    for (lst::const_iterator i = applyArgs.begin(); i != applyArgs.end(); ++i)
    {
        argsSubs.append((*i) == 1);
    }
    ex terms = src.expand();
    map<ex, ex, ex_is_less> preToPost;
    for (const_iterator term = terms.begin(); term != terms.end(); ++term)
    {
        ex post = term->subs(argsSubs);
        ex pre = (*term) / post;
        if (preToPost.find(pre) == preToPost.end())
            preToPost[pre] = post;
        else
            preToPost[pre] += post;
    }
    typedef map<ex, ex, ex_is_less>::iterator map_iter;
    ex result;
    int index = 0;
    for (map_iter prePost = preToPost.begin(); prePost != preToPost.end(); ++prePost)
    {
        stringstream symbolName;
        symbolName << tempName << index;
        symbol temp(symbolName.str());
        temps.push_back(make_pair(temp, prePost->first));
        result += temp * prePost->second;
        index ++;
    }
    return result;
}

ex partial2(const ex& src, const lst& restArgs, const string& tempName, vector<pair<symbol, ex>>& temps)
{
    lst argsSubs;
    for (lst::const_iterator i = restArgs.begin(); i != restArgs.end(); ++i)
    {
        argsSubs.append((*i) == 1);
    }
    ex terms = src.expand();
    map<ex, ex, ex_is_less> postToPre;
    for (const_iterator term = terms.begin(); term != terms.end(); ++term)
    {
        ex pre = term->subs(argsSubs);
        ex post = (*term) / pre;
        if (postToPre.find(post) == postToPre.end())
            postToPre[post] = pre;
        else
            postToPre[post] += pre;
    }
    typedef map<ex, ex, ex_is_less>::iterator map_iter;
    ex result;
    int index = 0;
    for (map_iter postPre = postToPre.begin(); postPre != postToPre.end(); ++postPre)
    {
        stringstream symbolName;
        symbolName << tempName << index;
        symbol temp(symbolName.str());
        temps.push_back(make_pair(temp, postPre->second));
        result += temp * postPre->first;
        index ++;
    }
    return result;
}

int main()
{
    // Variables
    ExColor1 s0("s0"), s1("s1"), s2("s2"), s3("s3");
    ExColor1 f0("f0"), f1("f1"), f2("f2"), f3("f3");
    ExColor1 b0("b0"), b1("b1"), b2("b2"), b3("b3");
    symbol u("u"), v("v");
    lst syms;
    syms = s0.rgbSymbol, s0.alphaSymbol,
        s1.rgbSymbol, s1.alphaSymbol,
        s2.rgbSymbol, s2.alphaSymbol,
        s3.rgbSymbol, s3.alphaSymbol,
        f0.rgbSymbol, f0.alphaSymbol,
        f1.rgbSymbol, f1.alphaSymbol,
        f2.rgbSymbol, f2.alphaSymbol,
        f3.rgbSymbol, f3.alphaSymbol,
        b0.rgbSymbol, b0.alphaSymbol,
        b1.rgbSymbol, b1.alphaSymbol,
        b2.rgbSymbol, b2.alphaSymbol,
        b3.rgbSymbol, b3.alphaSymbol,
        u, v;

    // Generate function
    ex uvError;
    ifstream inf("error.ginac");
    if (!inf.is_open())
    {
        Color<ex, ex> s = bilinear(s0, s1, s2, s3, ex(u), ex(v));
        Color<ex, ex> f = bilinear(f0, f1, f2, f3, ex(u), ex(v));
        Color<ex, ex> b = bilinear(b0, b1, b2, b3, ex(u), ex(v));
        ex error = iseabc(s, alphaBlend(f, b));
        ex uError = integral(u, 0, 1, error).eval_integ();
        uvError = integral(v, 0, 1, uError).eval_integ();
        ofstream of("error.ginac");
        of << archive(uvError);
        of.close();
    }
    else
    {
        archive arch;
        inf >> arch;
        uvError = arch.unarchive_ex(syms);
        inf.close();
    }

    // Partial apply
    typedef vector<pair<symbol, ex>>::const_iterator vec_iter;
    lst l0;
    l0 =  s0.rgbSymbol, s0.alphaSymbol,
        s1.rgbSymbol, s1.alphaSymbol,
        s2.rgbSymbol, s2.alphaSymbol,
        s3.rgbSymbol, s3.alphaSymbol,
        f0.rgbSymbol, f0.alphaSymbol,
        f1.rgbSymbol, f1.alphaSymbol,
        f2.rgbSymbol, f2.alphaSymbol,
        b0.rgbSymbol, b0.alphaSymbol,
        b1.rgbSymbol, b1.alphaSymbol,
        b2.rgbSymbol, b2.alphaSymbol;
    lst l1(f3.alphaSymbol, b3.alphaSymbol);
    lst l2(f3.rgbSymbol,  b3.rgbSymbol);
    lst l12(f3.alphaSymbol, b3.alphaSymbol, f3.rgbSymbol,  b3.rgbSymbol);

    vector<pair<symbol, ex>> temps0, temps1, result;
    ex part0 = partial2(uvError, l12, "temp0_", temps0);
    ex part1 = partial2(part0, l2, "temp1_", temps1);

    // Print class
    string tab = "    ";
    cout << csrc_double;
    cout << "class AdecError" << endl;
    cout << "{" << endl;
    cout << "public:" << endl;

    cout << tab << "// Needed for calcTemp0" << endl;
    for (lst::const_iterator i = l0.begin(); i != l0.end(); ++i)
        cout << tab << "double " << (*i) << ";" << endl;

    cout << tab << "// Needed for calcTemp1" << endl;
    for (lst::const_iterator i = l1.begin(); i != l1.end(); ++i)
        cout << tab << "double " << (*i) << ";" << endl;

    cout << tab << "// Needed for result" << endl;
    for (lst::const_iterator i = l2.begin(); i != l2.end(); ++i)
        cout << tab << "double " << (*i) << ";" << endl;

    cout << endl;

    cout << tab << "void calcTemp0 ()" << endl;
    cout << tab << "{" << endl;
    for (vec_iter i = temps0.begin(); i != temps0.end(); ++i)
        cout << tab << tab << i->first.get_name() << " = " << i->second << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "void calcTemp1 ()" << endl;
    cout << tab << "{" << endl;
    for (vec_iter i = temps1.begin(); i != temps1.end(); ++i)
        cout << tab << tab << i->first.get_name() << " = " << i->second << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "double calcResult ()" << endl;
    cout << tab << "{" << endl;
    cout << tab << tab << "return " << part1 << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << endl;
    cout << "private:" << endl;

    for (vec_iter i = temps0.begin(); i != temps0.end(); ++i)
        cout << tab << "double " << i->first.get_name() << ";" << endl;

    for (vec_iter i = temps1.begin(); i != temps1.end(); ++i)
        cout << tab << "double " << i->first.get_name() << ";" << endl;

    cout << "};" << endl;

    return 0;
}
