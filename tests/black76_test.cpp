#include "dp/black76.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

namespace {

dp::Black76Inputs atTheMoneyCall()
{
    return {.type = dp::OptionType::Call, .forward = 100.0, .strike = 100.0, .timeToExpiry = 1.0, .volatility = 0.2};
}

// Central finite difference of price() with respect to one input field.
double numericDerivative(dp::Black76Inputs inputs, double dp::Black76Inputs::*field, double bump)
{
    inputs.*field += bump;
    const double up = dp::price(inputs);
    inputs.*field -= 2.0 * bump;
    const double down = dp::price(inputs);
    return (up - down) / (2.0 * bump);
}

}  // namespace

TEST(Black76, MatchesReferenceValues)
{
    // Reference values computed independently (Python, math.erfc).
    EXPECT_NEAR(dp::price(atTheMoneyCall()), 7.965567455405804, 1e-10);

    const dp::Black76Inputs put{
        .type = dp::OptionType::Put, .forward = 60000.0, .strike = 65000.0, .timeToExpiry = 0.25, .volatility = 0.6};
    EXPECT_NEAR(dp::price(put), 10213.753483781838, 1e-7);
}

TEST(Black76, SatisfiesPutCallParity)
{
    for (const double strike : {50.0, 90.0, 100.0, 110.0, 200.0}) {
        dp::Black76Inputs call = atTheMoneyCall();
        call.strike = strike;
        call.discountRate = 0.05;
        dp::Black76Inputs put = call;
        put.type = dp::OptionType::Put;

        const double discountedForwardMinusStrike = std::exp(-0.05) * (call.forward - strike);
        EXPECT_NEAR(dp::price(call) - dp::price(put), discountedForwardMinusStrike, 1e-10) << "strike " << strike;
    }
}

TEST(Black76, ReturnsIntrinsicValueAtExpiry)
{
    dp::Black76Inputs call = atTheMoneyCall();
    call.forward = 120.0;
    call.timeToExpiry = 0.0;
    EXPECT_DOUBLE_EQ(dp::price(call), 20.0);

    call.type = dp::OptionType::Put;
    EXPECT_DOUBLE_EQ(dp::price(call), 0.0);
}

TEST(Black76, RejectsInvalidInputs)
{
    dp::Black76Inputs inputs = atTheMoneyCall();
    inputs.forward = 0.0;
    EXPECT_THROW(dp::price(inputs), std::invalid_argument);

    inputs = atTheMoneyCall();
    inputs.strike = -1.0;
    EXPECT_THROW(dp::price(inputs), std::invalid_argument);

    inputs = atTheMoneyCall();
    inputs.volatility = -0.1;
    EXPECT_THROW(dp::greeks(inputs), std::invalid_argument);
}

TEST(Black76, GreeksMatchFiniteDifferences)
{
    for (const dp::OptionType type : {dp::OptionType::Call, dp::OptionType::Put}) {
        dp::Black76Inputs inputs{
            .type = type, .forward = 80000.0, .strike = 90000.0, .timeToExpiry = 0.3, .volatility = 0.55,
            .discountRate = 0.03};
        const dp::Greeks g = dp::greeks(inputs);

        EXPECT_NEAR(g.delta, numericDerivative(inputs, &dp::Black76Inputs::forward, 1.0), 1e-6);
        EXPECT_NEAR(g.vega, numericDerivative(inputs, &dp::Black76Inputs::volatility, 1e-5), 1e-3);
        EXPECT_NEAR(g.theta, -numericDerivative(inputs, &dp::Black76Inputs::timeToExpiry, 1e-6), 1e-2);
        EXPECT_DOUBLE_EQ(g.vega, dp::vega(inputs));

        const double h = 10.0;
        dp::Black76Inputs up = inputs;
        up.forward += h;
        dp::Black76Inputs down = inputs;
        down.forward -= h;
        const double numericGamma = (dp::price(up) - 2.0 * dp::price(inputs) + dp::price(down)) / (h * h);
        EXPECT_NEAR(g.gamma, numericGamma, 1e-9);
    }
}

TEST(Black76, ConvertsUsdPriceToCoin)
{
    EXPECT_DOUBLE_EQ(dp::usdToCoin(5000.0, 100000.0), 0.05);
}
