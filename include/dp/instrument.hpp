/**
 * @file instrument.hpp
 * @brief Deribit option instrument names, e.g. "BTC-27DEC24-60000-C".
 */
#pragma once

#include "dp/option.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace dp {

struct Instrument {
    std::string currency;            ///< "BTC", "ETH", "XRP_USDC", ...
    std::chrono::sys_seconds expiry;  ///< Deribit options expire at 08:00 UTC.
    double strike = 0.0;
    OptionType type = OptionType::Call;
};

/**
 * @brief Parses a Deribit option name: CURRENCY-DMMMYY-STRIKE-{C|P}.
 *
 * Fractional strikes use 'd' as the decimal point ("XRP_USDC-30JAN26-0d625-C").
 * @return std::nullopt for futures, perpetuals or malformed names.
 */
std::optional<Instrument> parseInstrumentName(std::string_view name);

/// ACT/365 year fraction from @p from to @p to; negative if @p to is earlier.
double yearFraction(std::chrono::sys_seconds from, std::chrono::sys_seconds to);

}  // namespace dp
