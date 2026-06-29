#pragma once
#include <string>

struct Function1D {
    enum class Kind { Constant, Sine, Linear } kind = Kind::Constant;
    double a = 0.0;
    double b = 0.0;
    double c = 0.0;
    double d = 0.0;
    std::string source = "0";

    double eval(double t) const;
    bool parseFromText(const std::string& text);
};
