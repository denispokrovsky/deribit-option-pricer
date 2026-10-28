/**
 * @file quote_pricer.hpp
 * @brief Application service: turns a Deribit quote into implied volatility and Greeks.
 *
 * This is the only module that knows both the Deribit conventions and the
 * pricing model; the CLI just reads, calls priceQuote() and writes.
 */
#pragma once

#include "dp/black76.hpp"
#include "dp/implied_vol.hpp"
#include "dp/instrument.hpp"
#include "dp/quote_csv.hpp"

#include <string>
#include <string_view>
#include <variant>

namespace dp {

struct PricingResult {
    std::string instrumentName;
    Instrument instrument;
    double timeToExpiry = 0.0;      ///< Years.
    double forward = 0.0;           ///< USD.
    double priceUsd = 0.0;
    double priceCoin = 0.0;
    double impliedVol = 0.0;        ///< Our Black-76 volatility (0.6 = 60%).
    double deribitImpliedVol = 0.0; ///< Deribit mark_iv, same units, for comparison.
    int ivIterations = 0;
    Greeks greeks;
};

/// Why a quote could not be priced, in human-readable form.
struct PricingError {
    std::string reason;
};

using PricingOutcome = std::variant<PricingResult, PricingError>;

/// Implies volatility from the quote's mark price with @p solver and computes Greeks at it.
PricingOutcome priceQuote(const OptionQuote& quote, const ImpliedVolSolver& solver);

inline constexpr std::string_view kResultCsvHeader =
    "instrument_name,type,strike,expiry_years,forward,price_usd,price_coin,"
    "iv_model,iv_deribit,delta,gamma,vega,theta,iv_iterations";

std::string formatResultCsvRow(const PricingResult& result);

}  // namespace dp
