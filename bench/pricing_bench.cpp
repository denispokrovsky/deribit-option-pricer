// Performance checks for the requirements in docs/requirements.md (TR-P1..TR-P3)
// and the data behind the IV solver choice in docs/architecture.md.

#include "dp/black76.hpp"
#include "dp/implied_vol.hpp"
#include "dp/quote_pricer.hpp"

#include <benchmark/benchmark.h>

#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <vector>

namespace {

constexpr double kForward = 80000.0;

/// A synthetic but realistic chain: 10 expiries x 50 strikes x call/put = 1000 options.
std::vector<dp::IvProblem> makeChain()
{
    std::vector<dp::IvProblem> chain;
    for (int expiry = 1; expiry <= 10; ++expiry) {
        const double timeToExpiry = 0.02 * expiry * expiry;  // ~1 week .. 2 years
        for (int i = 0; i < 50; ++i) {
            const double strike = kForward * (0.5 + 0.02 * i);  // 50% .. 148% of forward
            const double smileVolatility = 0.45 + 0.4 * std::abs(strike / kForward - 1.0);
            for (const dp::OptionType type : {dp::OptionType::Call, dp::OptionType::Put}) {
                const double target = dp::price({.type = type,
                                                 .forward = kForward,
                                                 .strike = strike,
                                                 .timeToExpiry = timeToExpiry,
                                                 .volatility = smileVolatility});
                chain.push_back({.type = type,
                                 .forward = kForward,
                                 .strike = strike,
                                 .timeToExpiry = timeToExpiry,
                                 .targetPrice = target});
            }
        }
    }
    return chain;
}

const std::vector<dp::IvProblem>& chain()
{
    static const std::vector<dp::IvProblem> instance = makeChain();
    return instance;
}

void BM_Black76Price(benchmark::State& state)
{
    const dp::Black76Inputs inputs{
        .type = dp::OptionType::Call, .forward = kForward, .strike = 90000.0, .timeToExpiry = 0.25, .volatility = 0.5};
    for (auto _ : state) {
        benchmark::DoNotOptimize(dp::price(inputs));
    }
}
BENCHMARK(BM_Black76Price);

void BM_Black76Greeks(benchmark::State& state)
{
    const dp::Black76Inputs inputs{
        .type = dp::OptionType::Call, .forward = kForward, .strike = 90000.0, .timeToExpiry = 0.25, .volatility = 0.5};
    for (auto _ : state) {
        benchmark::DoNotOptimize(dp::greeks(inputs));
    }
}
BENCHMARK(BM_Black76Greeks);

/// Time per implied-volatility solve, averaged over the whole chain.
template <typename Solver>
void BM_ImpliedVol(benchmark::State& state)
{
    const Solver solver;
    const auto& problems = chain();
    std::int64_t iterations = 0;
    std::int64_t failures = 0;
    for (auto _ : state) {
        for (const dp::IvProblem& problem : problems) {
            dp::IvResult result = solver.solve(problem);
            iterations += result.iterations;
            failures += result.converged() ? 0 : 1;
            benchmark::DoNotOptimize(result);
        }
    }
    const auto solves = static_cast<double>(state.iterations() * static_cast<std::int64_t>(problems.size()));
    state.SetItemsProcessed(static_cast<std::int64_t>(solves));
    state.counters["avg_iterations"] = static_cast<double>(iterations) / solves;
    state.counters["failure_rate"] = static_cast<double>(failures) / solves;
}
BENCHMARK_TEMPLATE(BM_ImpliedVol, dp::BisectionSolver);
BENCHMARK_TEMPLATE(BM_ImpliedVol, dp::NewtonSolver);
BENCHMARK_TEMPLATE(BM_ImpliedVol, dp::SafeguardedNewtonSolver);

/// End-to-end: 1000 Deribit quotes -> parsed name, IV and Greeks (requirement TR-P3).
void BM_PriceChainOf1000Quotes(benchmark::State& state)
{
    const std::int64_t quoteTimeMs = 1793199626000;  // 2026-10-28
    const std::chrono::sys_seconds quoteTime{std::chrono::seconds{quoteTimeMs / 1000}};
    std::vector<dp::OptionQuote> quotes;
    for (const dp::IvProblem& problem : chain()) {
        // Express the synthetic expiry as a Deribit name; day precision is enough here.
        const auto expiryDay = std::chrono::floor<std::chrono::days>(quoteTime) +
                               std::chrono::days{1 + static_cast<int>(problem.timeToExpiry * 365)};
        const std::chrono::year_month_day date{expiryDay};
        constexpr const char* months[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                                          "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
        quotes.push_back({
            .instrumentName = std::format("BTC-{}{}{:02}-{}-{}", static_cast<unsigned>(date.day()),
                                          months[static_cast<unsigned>(date.month()) - 1],
                                          static_cast<int>(date.year()) % 100, problem.strike,
                                          problem.type == dp::OptionType::Call ? "C" : "P"),
            .underlyingPrice = problem.forward,
            .markPriceCoin = dp::usdToCoin(problem.targetPrice, problem.forward),
            .markIvPercent = 0.0,
            .timestampMs = quoteTimeMs,
        });
    }

    const dp::SafeguardedNewtonSolver solver;
    for (auto _ : state) {
        for (const dp::OptionQuote& quote : quotes) {
            benchmark::DoNotOptimize(dp::priceQuote(quote, solver));
        }
    }
    state.SetItemsProcessed(state.iterations() * static_cast<std::int64_t>(quotes.size()));
}
BENCHMARK(BM_PriceChainOf1000Quotes)->Unit(benchmark::kMillisecond);

}  // namespace

BENCHMARK_MAIN();
