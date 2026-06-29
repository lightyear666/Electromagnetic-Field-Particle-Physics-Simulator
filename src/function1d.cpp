#include "function1d.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <vector>

static std::string compact(std::string s) {
    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char ch) { return std::isspace(ch); }), s.end());
    return s;
}

static bool parseArgs(const std::string& body, std::vector<double>& out) {
    std::stringstream ss(body);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (item.empty()) return false;
        char* end = nullptr;
        double v = std::strtod(item.c_str(), &end);
        if (!end || *end != '\0') return false;
        out.push_back(v);
    }
    return true;
}

double Function1D::eval(double t) const {
    switch (kind) {
        case Kind::Constant: return a;
        case Kind::Sine: return a * std::sin(b * t + c) + d;
        case Kind::Linear: return a * t + b;
    }
    return 0.0;
}

bool Function1D::parseFromText(const std::string& text) {
    std::string s = compact(text);
    if (s.empty()) return false;

    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (end && *end == '\0') {
        kind = Kind::Constant;
        a = v;
        source = s;
        return true;
    }

    auto callBody = [&](const char* name, std::string& body) -> bool {
        std::string prefix = std::string(name) + "(";
        if (s.rfind(prefix, 0) != 0 || s.back() != ')') return false;
        body = s.substr(prefix.size(), s.size() - prefix.size() - 1);
        return true;
    };

    std::string body;
    std::vector<double> args;
    if (callBody("const", body) && parseArgs(body, args) && args.size() == 1) {
        kind = Kind::Constant;
        a = args[0];
        source = s;
        return true;
    }

    args.clear();
    if (callBody("sin", body) && parseArgs(body, args) && args.size() == 4) {
        kind = Kind::Sine;
        a = args[0];
        b = args[1];
        c = args[2];
        d = args[3];
        source = s;
        return true;
    }

    args.clear();
    if (callBody("linear", body) && parseArgs(body, args) && args.size() == 2) {
        kind = Kind::Linear;
        a = args[0];
        b = args[1];
        source = s;
        return true;
    }

    return false;
}
