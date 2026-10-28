/**
 * @file black76.hpp
 * @brief Black-76 pricing and Greeks for European options on a forward.
 *
 * Deribit marks its options with a Black-76 model on the underlying future
 * (or synthetic forward), with zero interest rate. Prices here are in the
 * quote currency (USD); use usdToCoin() for Deribit's inverse quotation.
 */
#pragma once

#include "dp/option.hpp"

namespace dp {

/// Inputs of the Black-76 model.
struct Black76Inputs {
    OptionType type = OptionType::Call;
    double forward = 0.0;       ///< Forward (futures) price of the underlying, USD.
    double strike = 0.0;        ///< Strike, USD.
    double timeToExpiry = 0.0;  ///< Year fraction until expiry (ACT/365).
    double volatility = 0.0;    ///< Annualised volatility: 0.6 means 60%.
    double discountRate = 0.0;  ///< Continuously compounded rate; Deribit uses 0.
};

/// Sensitivities of the USD option price.
struct Greeks {
    double delta = 0.0;  ///< dV/dF.
    double gamma = 0.0;  ///< d2V/dF2.
    double vega = 0.0;   ///< dV/dsigma per 1.00 (not per 1%) of volatility.
    double theta = 0.0;  ///< dV/dt per year of calendar time (usually negative).
};

/**
 * @brief Option price in USD.
 * @throws std::invalid_argument if forward or strike is not positive or
 *         volatility is negative.
 *
 * At or after expiry, or with zero volatility, returns the discounted
 * intrinsic value.
 */
double price(const Black76Inputs& inputs);

/// Vega alone; cheaper than greeks() when only vega is needed (IV solvers).
double vega(const Black76Inputs& inputs);

/// All Greeks of price(). Same preconditions as price().
Greeks greeks(const Black76Inputs& inputs);

/// Deribit quotes option premiums in the underlying coin: USD price / underlying price.
constexpr double usdToCoin(double usdPrice, double underlyingPrice) noexcept
{
    return usdPrice / underlyingPrice;
}

/// Standard normal cumulative distribution function.
double normalCdf(double x) noexcept;

/// Standard normal probability density function.
double normalPdf(double x) noexcept;

}  // namespace dp
