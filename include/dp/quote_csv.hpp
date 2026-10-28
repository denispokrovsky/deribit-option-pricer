/**
 * @file quote_csv.hpp
 * @brief CSV adapter for Deribit option quotes (input of dp_cli).
 *
 * The columns are a subset of the Deribit API response
 * public/get_book_summary_by_currency, as written by scripts/fetch_deribit.py.
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace dp {

/// One option quote as published by Deribit.
struct OptionQuote {
    std::string instrumentName;   ///< e.g. "BTC-27DEC24-60000-C".
    double underlyingPrice = 0.0;  ///< Forward used by Deribit for this expiry, USD.
    double markPriceCoin = 0.0;   ///< Mark price in the underlying coin (inverse quote).
    double markIvPercent = 0.0;   ///< Deribit's mark implied volatility, in percent.
    std::int64_t timestampMs = 0;  ///< Quote time, Unix epoch milliseconds.
};

inline constexpr std::string_view kQuoteCsvHeader =
    "instrument_name,underlying_price,mark_price,mark_iv,creation_timestamp";

/// Parses one data row; returns std::nullopt if it is malformed.
std::optional<OptionQuote> parseQuoteCsvRow(std::string_view line);

}  // namespace dp
