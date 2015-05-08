#include <iostream>
#include <sstream>
#include <ginac/ginac.h>
#include <ginac/archive.h>
#include <regex>

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

string csrc_long(ex e)
{
    ostringstream ss;
    ss << e;
    regex re("([a-zA-Z0-9_]+)\\^2");
    return regex_replace(ss.str(), re, "($1*$1)");
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

struct ExColor1 : public Color<ex, ex>
{
public:
    const symbol rgbSymbol;
    const symbol alphaSymbol;
    const int max;
    ExColor1(const string& name, int mx) :
        Color(),
        rgbSymbol(string("C_").append(name)),
        alphaSymbol(string("A_").append(name)),
        max(mx) {
            rgb = ex(rgbSymbol) / mx;
            alpha = ex(alphaSymbol) / mx;
        }
    void appendMaxSubs(lst& l)
    {
        l.append(rgbSymbol == max);
        l.append(alphaSymbol == max);
    }
};

int main()
{
    // Variables
    ExColor1 s0("s0", 255), s1("s1", 255), s2("s2", 255), s3("s3", 255);
    ExColor1 f0("f0", 15), f1("f1", 15), f2("f2", 15), f3("f3", 15);
    ExColor1 b0("b0", 15), b1("b1", 15), b2("b2", 15), b3("b3", 15);
    symbol u("u"), v("v");
    vector<ExColor1> colors = {
        s0, s1, s2, s3,
        f0, f1, f2, f3,
        b0, b1, b2, b3,
    };

    // Generate function
    Color<ex, ex> s = bilinear(s0, s1, s2, s3, ex(u), ex(v));
    Color<ex, ex> f = bilinear(f0, f1, f2, f3, ex(u), ex(v));
    Color<ex, ex> b = bilinear(b0, b1, b2, b3, ex(u), ex(v));
    ex error = iseabc(s, alphaBlend(f, b));
    ex uError = integral(u, 0, 1, error).eval_integ();
    ex uvError = integral(v, 0, 1, uError).eval_integ();
    ex uvError2 = uvError * uvError.denom();

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
    ex part0 = partial(uvError2, l0, "temp0_", temp0);
    ex part1 = partial2(part0, l23, "temp1_", temp1);
    ex part2 = partial2(part1, l3, "temp2_", temp2);

    // Print class
    string tab = "    ";
    cout << "class AdecError" << endl;
    cout << "{" << endl;
    cout << "public:" << endl;

    string inputType = "long";

    cout << tab << "// Needed for calcTemp0" << endl;
    for (const auto & i : l0)
        cout << tab << inputType << " " << (i) << ";" << endl;

    cout << tab << "// Needed for calcTemp1" << endl;
    for (const auto & i : l1)
        cout << tab << inputType << " " << (i) << ";" << endl;

    cout << tab << "// Needed for calcTemp2" << endl;
    for (const auto & i : l2)
        cout << tab << inputType << " " << (i) << ";" << endl;

    cout << tab << "// Needed for result" << endl;
    for (const auto & i : l3)
        cout << tab << inputType << " " << (i) << ";" << endl;

    cout << endl;

    cout << tab << "void calcTemp0 ()" << endl;
    cout << tab << "{" << endl;
    for (const auto & i : temp0)
        cout << tab << tab << i.first.get_name() << " = " << csrc_long(i.second.eval()) << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "void copyTemp0 (AdecError& x)" << endl;
    cout << tab << "{" << endl;
    for (const auto & i : l0)
        cout << tab << tab << i << " = x." << i << ";" << endl;
    for (const auto & i : temp0)
        cout << tab << tab << i.first.get_name() << " = x." << i.first.get_name() << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "void calcTemp1 ()" << endl;
    cout << tab << "{" << endl;
    for (const auto & i : temp1)
        cout << tab << tab << i.first.get_name() << " = " << csrc_long(i.second.eval()) << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "void calcTemp2 ()" << endl;
    cout << tab << "{" << endl;
    for (const auto & i : temp2)
        cout << tab << tab << i.first.get_name() << " = " << csrc_long(i.second.eval()) << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << tab << "long calcResult ()" << endl;
    cout << tab << "{" << endl;
    cout << tab << tab << "return " << csrc_long(part2) << ";" << endl;
    cout << tab << "}" << endl << endl;

    cout << endl;
    cout << "private:" << endl;
    const string tempType = "long";
    for (const auto & i : temp0)
        cout << tab << tempType << " " << i.first.get_name() << ";" << endl;

    for (const auto & i : temp1)
        cout << tab << tempType << " " << i.first.get_name() << ";" << endl;

    for (const auto & i : temp2)
        cout << tab << tempType << " " << i.first.get_name() << ";" << endl;

    cout << "};" << endl;

    return 0;
}
