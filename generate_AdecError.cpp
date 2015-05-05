#include <iostream>
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
    return aRgbError * (aRgbError - aError) + (aError * aError) / 3;
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

lst concat(lst a, lst b)
{
    lst::const_iterator i;
    lst c;
    for (i = a.begin(); i != a.end(); ++i)
        c.append(*i);
    for (i = b.begin(); i != b.end(); ++i)
        c.append(*i);
    return c;
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
    Color<ex, ex> s = bilinear(s0, s1, s2, s3, ex(u), ex(v));
    Color<ex, ex> f = bilinear(f0, f1, f2, f3, ex(u), ex(v));
    Color<ex, ex> b = bilinear(b0, b1, b2, b3, ex(u), ex(v));
    ex error = iseabc(s, alphaBlend(f, b));
    ex uError = integral(u, 0, 1, error).eval_integ();
    ex uvError = integral(v, 0, 1, uError).eval_integ();

    // Partial apply
    typedef vector<pair<symbol, ex>>::const_iterator vec_iter;
    lst l0, l1, l2, l3;
    l0 = s0.alphaSymbol, s1.alphaSymbol, s2.alphaSymbol, s3.alphaSymbol,
        f0.alphaSymbol, f1.alphaSymbol, f2.alphaSymbol,
        b0.alphaSymbol, b1.alphaSymbol, b2.alphaSymbol;
    l1 = s0.rgbSymbol, s1.rgbSymbol, s2.rgbSymbol, s3.rgbSymbol,
        f0.rgbSymbol, f1.rgbSymbol, f2.rgbSymbol,
        b0.rgbSymbol, b1.rgbSymbol, b2.rgbSymbol;
    l2 = f3.alphaSymbol, b3.alphaSymbol;
    l3 = f3.rgbSymbol,  b3.rgbSymbol;

    lst l23, l123;
    l23 = concat(l2, l3);
    l123 = concat(l1, l23);
    vector<pair<symbol, ex>> temp0, temp1, temp2;
    ex part0 = partial(uvError, l0, "temp0_", temp0);
    ex part1 = partial2(part0, l23, "temp1_", temp1);
    ex part2 = partial2(part1, l3, "temp2_", temp2);

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

    cout << tab << "// Needed for calcTemp2" << endl;
    for (lst::const_iterator i = l2.begin(); i != l2.end(); ++i)
        cout << tab << "double " << (*i) << ";" << endl;

    cout << tab << "// Needed for result" << endl;
    for (lst::const_iterator i = l3.begin(); i != l3.end(); ++i)
        cout << tab << "double " << (*i) << ";" << endl;

    cout << endl;

    cout << tab << "void calcTemp0 ()" << endl;
    cout << tab << "{" << endl;
    for (vec_iter i = temp0.begin(); i != temp0.end(); ++i)
        cout << tab << tab << i->first.get_name() << " = " << i->second << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "void copyTemp0 (AdecError& x)" << endl;
    cout << tab << "{" << endl;
    for (vec_iter i = temp0.begin(); i != temp0.end(); ++i)
        cout << tab << tab << i->first.get_name() << " = x." << i->first.get_name() << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "void calcTemp1 ()" << endl;
    cout << tab << "{" << endl;
    for (vec_iter i = temp1.begin(); i != temp1.end(); ++i)
        cout << tab << tab << i->first.get_name() << " = " << i->second << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "void calcTemp2 ()" << endl;
    cout << tab << "{" << endl;
    for (vec_iter i = temp2.begin(); i != temp2.end(); ++i)
        cout << tab << tab << i->first.get_name() << " = " << i->second << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "double calcResult ()" << endl;
    cout << tab << "{" << endl;
    cout << tab << tab << "return " << part2 << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << endl;
    cout << "private:" << endl;

    for (vec_iter i = temp0.begin(); i != temp0.end(); ++i)
        cout << tab << "double " << i->first.get_name() << ";" << endl;

    for (vec_iter i = temp1.begin(); i != temp1.end(); ++i)
        cout << tab << "double " << i->first.get_name() << ";" << endl;

    for (vec_iter i = temp2.begin(); i != temp2.end(); ++i)
        cout << tab << "double " << i->first.get_name() << ";" << endl;

    cout << "};" << endl;

    return 0;
}
