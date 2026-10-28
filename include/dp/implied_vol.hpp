/**
 * @file implied_vol.hpp
 * @brief Implied volatility solvers (Strategy pattern) and their factory.
 *
 * Three interchangeable algorithms are provided so they can be compared in
 * the benchmark; see docs/architecture.md for the reasoning behind the
 * default choice (SafeguardedNewtonSolver).
 */
#pragma once

#include "dp/option.hpp"

#include <memory>
#include <span>
#include <string_view>

namespace dp {

/// A market price for which we look for the Black-76 volatility.
struct IvProblem {
    OptionType type = OptionType::Call;
    double forward = 0.0;       ///< USD.
    double strike = 0.0;        ///< USD.
    double timeToExpiry = 0.0;  ///< Years, must be positive.
    double targetPrice = 0.0;   ///< Observed option price, USD.
    double discountRate = 0.0;
};

enum class IvStatus {
    Converged,
    NotConverged,      ///< Iteration limit hit, or the method diverged.
    PriceOutOfBounds,  ///< Price violates no-arbitrage bounds: no volatility fits.
};

struct IvResult {
    double volatility = 0.0;  ///< NaN unless status is Converged.
    int iterations = 0;
    IvStatus status = IvStatus::NotConverged;

    bool converged() const noexcept { return status == IvStatus::Converged; }
};

struct IvSettings {
    double minVolatility = 1e-6;
    double maxVolatility = 10.0;  ///< 1000%: far above anything seen on Deribit.
    double tolerance = 1e-10;     ///< Stop when the volatility step is smaller.
    int maxIterations = 100;
};

/// Strategy interface: one algorithm for inverting the Black-76 price.
class ImpliedVolSolver {
public:
    virtual ~ImpliedVolSolver() = default;
    virtual IvResult solve(const IvProblem& problem) const = 0;
    virtual std::string_view name() const noexcept = 0;
};

/// Always converges, but needs ~37 iterations for 1e-10 precision.
class BisectionSolver final : public ImpliedVolSolver {
public:
    explicit BisectionSolver(IvSettings settings = {}) : settings_(settings) {}
    IvResult solve(const IvProblem& problem) const override;
    std::string_view name() const noexcept override { return "bisection"; }

private:
    IvSettings settings_;
};

/// Plain Newton-Raphson: very fast near the money, may diverge far from it.
class NewtonSolver final : public ImpliedVolSolver {
public:
    explicit NewtonSolver(IvSettings settings = {}) : settings_(settings) {}
    IvResult solve(const IvProblem& problem) const override;
    std::string_view name() const noexcept override { return "newton"; }

private:
    IvSettings settings_;
};

/// Newton-Raphson kept inside a shrinking bracket, falling back to bisection
/// whenever a Newton step would leave it. Fast and always converges.
class SafeguardedNewtonSolver final : public ImpliedVolSolver {
public:
    explicit SafeguardedNewtonSolver(IvSettings settings = {}) : settings_(settings) {}
    IvResult solve(const IvProblem& problem) const override;
    std::string_view name() const noexcept override { return "safeguarded"; }

private:
    IvSettings settings_;
};

/// Names accepted by makeSolver(); the first one is the default.
std::span<const std::string_view> solverNames() noexcept;

/// Factory: creates a solver by name, or returns nullptr for an unknown name.
std::unique_ptr<ImpliedVolSolver> makeSolver(std::string_view name);

}  // namespace dp
