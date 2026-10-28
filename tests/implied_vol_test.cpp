#include "dp/black76.hpp"
#include "dp/implied_vol.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <memory>
#include <string>
#include <string_view>

namespace {

dp::IvProblem problemFor(dp::OptionType type, double strike, double timeToExpiry, double volatility)
{
    const double forward = 80000.0;
    const double target = dp::price(
        {.type = type, .forward = forward, .strike = strike, .timeToExpiry = timeToExpiry, .volatility = volatility});
    return {.type = type, .forward = forward, .strike = strike, .timeToExpiry = timeToExpiry, .targetPrice = target};
}

// Every solver created by the factory must pass the same contract tests.
class ImpliedVolSolverTest : public ::testing::TestWithParam<std::string_view> {
protected:
    std::unique_ptr<dp::ImpliedVolSolver> solver = dp::makeSolver(GetParam());
};

}  // namespace

TEST_P(ImpliedVolSolverTest, RecoversVolatilityUsedToPrice)
{
    ASSERT_NE(solver, nullptr);
    for (const dp::OptionType type : {dp::OptionType::Call, dp::OptionType::Put}) {
        for (const double strike : {64000.0, 76000.0, 80000.0, 84000.0, 96000.0}) {
            for (const double volatility : {0.25, 0.5, 0.9}) {
                const dp::IvResult result = solver->solve(problemFor(type, strike, 0.25, volatility));
                ASSERT_TRUE(result.converged()) << solver->name() << " strike " << strike << " vol " << volatility;
                EXPECT_NEAR(result.volatility, volatility, 1e-7);
            }
        }
    }
}

TEST_P(ImpliedVolSolverTest, RejectsPricesOutsideArbitrageBounds)
{
    dp::IvProblem problem = problemFor(dp::OptionType::Call, 70000.0, 0.25, 0.5);

    problem.targetPrice = 9000.0;  // below intrinsic value of 10000
    EXPECT_EQ(solver->solve(problem).status, dp::IvStatus::PriceOutOfBounds);

    problem.targetPrice = 90000.0;  // a call can never be worth more than the forward
    EXPECT_EQ(solver->solve(problem).status, dp::IvStatus::PriceOutOfBounds);

    problem.timeToExpiry = 0.0;
    EXPECT_EQ(solver->solve(problem).status, dp::IvStatus::PriceOutOfBounds);
    EXPECT_TRUE(std::isnan(solver->solve(problem).volatility));
}

INSTANTIATE_TEST_SUITE_P(AllSolvers, ImpliedVolSolverTest, ::testing::ValuesIn(dp::solverNames()),
                         [](const auto& paramInfo) { return std::string{paramInfo.param}; });

TEST(SafeguardedNewtonSolver, ConvergesWhereVegaIsTiny)
{
    // Deep out of the money, short expiry: tiny vega makes the problem ill-conditioned.
    const dp::IvProblem problem = problemFor(dp::OptionType::Call, 120000.0, 0.02, 0.8);
    const dp::IvResult result = dp::SafeguardedNewtonSolver{}.solve(problem);
    ASSERT_TRUE(result.converged());
    EXPECT_NEAR(result.volatility, 0.8, 1e-6);
}

TEST(SafeguardedNewtonSolver, NeedsFewerIterationsThanBisection)
{
    const dp::IvProblem problem = problemFor(dp::OptionType::Put, 76000.0, 0.25, 0.5);
    EXPECT_LT(dp::SafeguardedNewtonSolver{}.solve(problem).iterations, dp::BisectionSolver{}.solve(problem).iterations);
}

TEST(SolverFactory, ReturnsNullForUnknownName)
{
    EXPECT_EQ(dp::makeSolver("secant"), nullptr);
    EXPECT_EQ(dp::makeSolver("bisection")->name(), "bisection");
}
