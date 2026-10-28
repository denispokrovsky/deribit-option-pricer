#include "dp/black76.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace dp {

namespace {

void validate(const Black76Inputs& in)
{
    // Negated comparisons also reject NaN.
    if (!(in.forward > 0.0)) {
        throw std::invalid_argument("Black-76: forward must be positive");
    }
    if (!(in.strike > 0.0)) {
        throw std::invalid_argument("Black-76: strike must be positive");
    }
    if (!(in.volatility >= 0.0)) {
        throw std::invalid_argument("Black-76: volatility must be non-negative");
    }
    if (std::isnan(in.timeToExpiry)) {
        throw std::invalid_argument("Black-76: time to expiry is NaN");
    }
}

/// No randomness left: the option is worth its discounted intrinsic value.
bool isDeterministic(const Black76Inputs& in)
{
    return in.timeToExpiry <= 0.0 || in.volatility == 0.0;
}

double discountFactor(const Black76Inputs& in)
{
    return std::exp(-in.discountRate * std::max(in.timeToExpiry, 0.0));
}

double intrinsic(const Black76Inputs& in)
{
    return std::max(sign(in.type) * (in.forward - in.strike), 0.0);
}

struct D1D2 {
    double d1;
    double d2;
};

D1D2 computeD1D2(const Black76Inputs& in)
{
    const double stdDev = in.volatility * std::sqrt(in.timeToExpiry);
    const double d1 = (std::log(in.forward / in.strike) + 0.5 * stdDev * stdDev) / stdDev;
    return {d1, d1 - stdDev};
}

}  // namespace

double normalCdf(double x) noexcept
{
    return 0.5 * std::erfc(-x / std::numbers::sqrt2);
}

double normalPdf(double x) noexcept
{
    constexpr double invSqrt2Pi = std::numbers::inv_sqrtpi / std::numbers::sqrt2;
    return invSqrt2Pi * std::exp(-0.5 * x * x);
}

double price(const Black76Inputs& in)
{
    validate(in);
    const double df = discountFactor(in);
    if (isDeterministic(in)) {
        return df * intrinsic(in);
    }
    const double w = sign(in.type);
    const auto [d1, d2] = computeD1D2(in);
    return df * w * (in.forward * normalCdf(w * d1) - in.strike * normalCdf(w * d2));
}

double vega(const Black76Inputs& in)
{
    validate(in);
    if (isDeterministic(in)) {
        return 0.0;
    }
    const auto [d1, d2] = computeD1D2(in);
    return discountFactor(in) * in.forward * normalPdf(d1) * std::sqrt(in.timeToExpiry);
}

Greeks greeks(const Black76Inputs& in)
{
    validate(in);
    const double df = discountFactor(in);
    const double w = sign(in.type);
    if (isDeterministic(in)) {
        return {.delta = intrinsic(in) > 0.0 ? df * w : 0.0};
    }

    const auto [d1, d2] = computeD1D2(in);
    const double sqrtT = std::sqrt(in.timeToExpiry);
    const double density = normalPdf(d1);
    const double undiscounted = w * (in.forward * normalCdf(w * d1) - in.strike * normalCdf(w * d2));

    return {
        .delta = df * w * normalCdf(w * d1),
        .gamma = df * density / (in.forward * in.volatility * sqrtT),
        .vega = df * in.forward * density * sqrtT,
        // Calendar time runs opposite to time to expiry: theta = -dV/dT.
        .theta = -df * in.forward * density * in.volatility / (2.0 * sqrtT)
                 + in.discountRate * df * undiscounted,
    };
}

}  // namespace dp
