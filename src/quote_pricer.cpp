#include "dp/quote_pricer.hpp"

#include <format>

namespace dp {

namespace {

std::string describe(IvStatus status)
{
    switch (status) {
    case IvStatus::Converged:
        return "converged";
    case IvStatus::NotConverged:
        return "implied volatility did not converge";
    case IvStatus::PriceOutOfBounds:
        return "mark price is outside no-arbitrage bounds";
    }
    return "unknown solver status";
}

}  // namespace

PricingOutcome priceQuote(const OptionQuote& quote, const ImpliedVolSolver& solver)
{
    const auto instrument = parseInstrumentName(quote.instrumentName);
    if (!instrument) {
        return PricingError{"not a Deribit option name"};
    }
    if (!(quote.underlyingPrice > 0.0) || !(quote.markPriceCoin >= 0.0)) {
        return PricingError{"non-positive underlying price or negative mark price"};
    }

    const std::chrono::sys_seconds quoteTime{
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds{quote.timestampMs})};
    const double timeToExpiry = yearFraction(quoteTime, instrument->expiry);
    if (!(timeToExpiry > 0.0)) {
        return PricingError{"option has expired"};
    }

    const double forward = quote.underlyingPrice;
    const double priceUsd = quote.markPriceCoin * forward;
    const IvResult iv = solver.solve({
        .type = instrument->type,
        .forward = forward,
        .strike = instrument->strike,
        .timeToExpiry = timeToExpiry,
        .targetPrice = priceUsd,
    });
    if (!iv.converged()) {
        return PricingError{describe(iv.status)};
    }

    return PricingResult{
        .instrumentName = quote.instrumentName,
        .instrument = *instrument,
        .timeToExpiry = timeToExpiry,
        .forward = forward,
        .priceUsd = priceUsd,
        .priceCoin = quote.markPriceCoin,
        .impliedVol = iv.volatility,
        .deribitImpliedVol = quote.markIvPercent / 100.0,
        .ivIterations = iv.iterations,
        .greeks = greeks({
            .type = instrument->type,
            .forward = forward,
            .strike = instrument->strike,
            .timeToExpiry = timeToExpiry,
            .volatility = iv.volatility,
        }),
    };
}

std::string formatResultCsvRow(const PricingResult& r)
{
    return std::format("{},{},{},{:.6f},{},{:.4f},{:.8f},{:.6f},{:.4f},{:.6f},{:.4e},{:.4f},{:.4f},{}",
                       r.instrumentName, toString(r.instrument.type), r.instrument.strike, r.timeToExpiry,
                       r.forward, r.priceUsd, r.priceCoin, r.impliedVol, r.deribitImpliedVol, r.greeks.delta,
                       r.greeks.gamma, r.greeks.vega, r.greeks.theta, r.ivIterations);
}

}  // namespace dp
