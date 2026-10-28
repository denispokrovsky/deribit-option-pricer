#include "dp/implied_vol.hpp"

#include "dp/black76.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace dp {

namespace {

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

Black76Inputs inputsAt(const IvProblem& p, double volatility)
{
    return {
        .type = p.type,
        .forward = p.forward,
        .strike = p.strike,
        .timeToExpiry = p.timeToExpiry,
        .volatility = volatility,
        .discountRate = p.discountRate,
    };
}

/// Price minus target: increasing in volatility, its root is the implied volatility.
double pricingError(const IvProblem& p, double volatility)
{
    return price(inputsAt(p, volatility)) - p.targetPrice;
}

/// A volatility exists only for prices strictly between the discounted
/// intrinsic value and the discounted forward (calls) or strike (puts).
bool isWithinArbitrageBounds(const IvProblem& p)
{
    if (!(p.timeToExpiry > 0.0 && p.forward > 0.0 && p.strike > 0.0)) {
        return false;
    }
    const double df = std::exp(-p.discountRate * p.timeToExpiry);
    const double intrinsic = df * std::max(sign(p.type) * (p.forward - p.strike), 0.0);
    const double upperBound = df * (p.type == OptionType::Call ? p.forward : p.strike);
    return p.targetPrice > intrinsic && p.targetPrice < upperBound;
}

/// The search interval must contain the root, i.e. the error changes sign on it.
bool bracketContainsRoot(const IvProblem& p, const IvSettings& s)
{
    return pricingError(p, s.minVolatility) <= 0.0 && pricingError(p, s.maxVolatility) >= 0.0;
}

/// Manaster-Koehler starting point: the volatility at which the option's
/// vega is largest, so Newton steps from it are well behaved.
double initialGuess(const IvProblem& p, const IvSettings& s)
{
    const double guess = std::sqrt(2.0 * std::abs(std::log(p.forward / p.strike)) / p.timeToExpiry);
    constexpr double atTheMoneyGuess = 0.5;
    return std::clamp(guess > 0.0 ? guess : atTheMoneyGuess, s.minVolatility, s.maxVolatility);
}

IvResult failure(IvStatus status, int iterations = 0)
{
    return {.volatility = kNaN, .iterations = iterations, .status = status};
}

constexpr std::array<std::string_view, 3> kSolverNames = {"safeguarded", "newton", "bisection"};

}  // namespace

IvResult BisectionSolver::solve(const IvProblem& p) const
{
    if (!isWithinArbitrageBounds(p)) {
        return failure(IvStatus::PriceOutOfBounds);
    }
    if (!bracketContainsRoot(p, settings_)) {
        return failure(IvStatus::NotConverged);
    }

    double low = settings_.minVolatility;
    double high = settings_.maxVolatility;
    for (int iteration = 1; iteration <= settings_.maxIterations; ++iteration) {
        const double middle = 0.5 * (low + high);
        if (pricingError(p, middle) < 0.0) {
            low = middle;
        } else {
            high = middle;
        }
        if (high - low < settings_.tolerance) {
            return {.volatility = 0.5 * (low + high), .iterations = iteration, .status = IvStatus::Converged};
        }
    }
    return failure(IvStatus::NotConverged, settings_.maxIterations);
}

IvResult NewtonSolver::solve(const IvProblem& p) const
{
    if (!isWithinArbitrageBounds(p)) {
        return failure(IvStatus::PriceOutOfBounds);
    }

    double volatility = initialGuess(p, settings_);
    for (int iteration = 1; iteration <= settings_.maxIterations; ++iteration) {
        const Black76Inputs inputs = inputsAt(p, volatility);
        const double slope = vega(inputs);
        if (!(slope > 0.0)) {
            return failure(IvStatus::NotConverged, iteration);
        }
        const double step = (price(inputs) - p.targetPrice) / slope;
        volatility -= step;
        if (!(volatility > 0.0) || !std::isfinite(volatility)) {
            return failure(IvStatus::NotConverged, iteration);
        }
        if (std::abs(step) < settings_.tolerance) {
            return {.volatility = volatility, .iterations = iteration, .status = IvStatus::Converged};
        }
    }
    return failure(IvStatus::NotConverged, settings_.maxIterations);
}

IvResult SafeguardedNewtonSolver::solve(const IvProblem& p) const
{
    if (!isWithinArbitrageBounds(p)) {
        return failure(IvStatus::PriceOutOfBounds);
    }
    if (!bracketContainsRoot(p, settings_)) {
        return failure(IvStatus::NotConverged);
    }

    double low = settings_.minVolatility;
    double high = settings_.maxVolatility;
    double volatility = initialGuess(p, settings_);
    for (int iteration = 1; iteration <= settings_.maxIterations; ++iteration) {
        const Black76Inputs inputs = inputsAt(p, volatility);
        const double error = price(inputs) - p.targetPrice;
        // The root stays inside [low, high]: shrink the bracket from the side we are on.
        if (error > 0.0) {
            high = volatility;
        } else {
            low = volatility;
        }

        const double slope = vega(inputs);
        double next = slope > 0.0 ? volatility - error / slope : kNaN;
        const bool newtonStepIsSafe = next > low && next < high;  // false for NaN
        if (!newtonStepIsSafe) {
            next = 0.5 * (low + high);
        }

        if (std::abs(next - volatility) < settings_.tolerance) {
            return {.volatility = next, .iterations = iteration, .status = IvStatus::Converged};
        }
        volatility = next;
    }
    return failure(IvStatus::NotConverged, settings_.maxIterations);
}

std::span<const std::string_view> solverNames() noexcept
{
    return kSolverNames;
}

std::unique_ptr<ImpliedVolSolver> makeSolver(std::string_view name)
{
    if (name == "safeguarded") {
        return std::make_unique<SafeguardedNewtonSolver>();
    }
    if (name == "newton") {
        return std::make_unique<NewtonSolver>();
    }
    if (name == "bisection") {
        return std::make_unique<BisectionSolver>();
    }
    return nullptr;
}

}  // namespace dp
