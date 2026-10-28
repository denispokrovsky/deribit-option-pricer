// Deribit conventions: instrument names, CSV quotes and the end-to-end quote pricer.

#include "dp/instrument.hpp"
#include "dp/quote_csv.hpp"
#include "dp/quote_pricer.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <string_view>
#include <variant>

using namespace std::chrono;

TEST(InstrumentName, ParsesCallWithTwoDigitDay)
{
    const auto instrument = dp::parseInstrumentName("BTC-27DEC24-60000-C");
    ASSERT_TRUE(instrument.has_value());
    EXPECT_EQ(instrument->currency, "BTC");
    EXPECT_EQ(instrument->expiry, sys_days{2024y / December / 27} + 8h);
    EXPECT_DOUBLE_EQ(instrument->strike, 60000.0);
    EXPECT_EQ(instrument->type, dp::OptionType::Call);
}

TEST(InstrumentName, ParsesPutWithOneDigitDayAndFractionalStrike)
{
    const auto instrument = dp::parseInstrumentName("XRP_USDC-3JAN25-0d625-P");
    ASSERT_TRUE(instrument.has_value());
    EXPECT_EQ(instrument->expiry, sys_days{2025y / January / 3} + 8h);
    EXPECT_DOUBLE_EQ(instrument->strike, 0.625);
    EXPECT_EQ(instrument->type, dp::OptionType::Put);
}

TEST(InstrumentName, RejectsNonOptionsAndMalformedNames)
{
    for (const char* name : {"BTC-PERPETUAL", "BTC-27DEC24", "BTC-31FEB25-60000-C", "BTC-27XYZ24-60000-C",
                             "BTC-27DEC24-60000-X", "BTC-27DEC24-abc-C", ""}) {
        EXPECT_FALSE(dp::parseInstrumentName(name).has_value()) << name;
    }
}

TEST(YearFraction, UsesActual365)
{
    const sys_seconds start = sys_days{2025y / January / 1};
    EXPECT_DOUBLE_EQ(dp::yearFraction(start, start + days{365}), 1.0);
    EXPECT_LT(dp::yearFraction(start + days{1}, start), 0.0);
}

TEST(QuoteCsv, ParsesRowIncludingWindowsLineEnding)
{
    const auto quote = dp::parseQuoteCsvRow("BTC-13NOV26-72000-C,80733.66,0.11224857,43.3,1793199626463\r");
    ASSERT_TRUE(quote.has_value());
    EXPECT_EQ(quote->instrumentName, "BTC-13NOV26-72000-C");
    EXPECT_DOUBLE_EQ(quote->underlyingPrice, 80733.66);
    EXPECT_DOUBLE_EQ(quote->markPriceCoin, 0.11224857);
    EXPECT_DOUBLE_EQ(quote->markIvPercent, 43.3);
    EXPECT_EQ(quote->timestampMs, 1793199626463);
}

TEST(QuoteCsv, RejectsMalformedRows)
{
    EXPECT_FALSE(dp::parseQuoteCsvRow("BTC-13NOV26-72000-C,80733.66,0.11"));
    EXPECT_FALSE(dp::parseQuoteCsvRow("BTC-13NOV26-72000-C,80733.66,abc,43.3,1793199626463"));
    EXPECT_FALSE(dp::parseQuoteCsvRow(",80733.66,0.1,43.3,1793199626463"));
}

// Real Deribit quotes (data/btc_options_sample.csv): our Black-76 volatility
// must reproduce Deribit's own mark_iv.
TEST(QuotePricer, ReproducesDeribitMarkIv)
{
    const dp::SafeguardedNewtonSolver solver;
    for (const char* row : {"BTC-13NOV26-72000-C,80733.66,0.11224857,43.3,1793199626463",
                            "BTC-13NOV26-70000-P,80733.21,0.00272602,46.37,1793199626467"}) {
        const auto outcome = dp::priceQuote(*dp::parseQuoteCsvRow(row), solver);
        const auto* result = std::get_if<dp::PricingResult>(&outcome);
        ASSERT_NE(result, nullptr) << row;
        EXPECT_NEAR(result->impliedVol, result->deribitImpliedVol, 0.0005) << row;
        EXPECT_NEAR(result->priceCoin * result->forward, result->priceUsd, 1e-9);
    }
}

TEST(QuotePricer, ReportsExpiredOptionsAndNonOptions)
{
    const dp::SafeguardedNewtonSolver solver;
    dp::OptionQuote quote{.instrumentName = "BTC-27DEC24-60000-C",
                          .underlyingPrice = 80000.0,
                          .markPriceCoin = 0.1,
                          .markIvPercent = 50.0,
                          .timestampMs = 1793199626463};
    EXPECT_TRUE(std::holds_alternative<dp::PricingError>(dp::priceQuote(quote, solver)));

    quote.instrumentName = "BTC-PERPETUAL";
    EXPECT_TRUE(std::holds_alternative<dp::PricingError>(dp::priceQuote(quote, solver)));
}

TEST(QuotePricer, FormatsOneCsvColumnPerHeaderColumn)
{
    const dp::SafeguardedNewtonSolver solver;
    const auto outcome =
        dp::priceQuote(*dp::parseQuoteCsvRow("BTC-13NOV26-72000-C,80733.66,0.11224857,43.3,1793199626463"), solver);
    const std::string row = dp::formatResultCsvRow(std::get<dp::PricingResult>(outcome));

    const auto commas = [](std::string_view s) { return std::ranges::count(s, ','); };
    EXPECT_EQ(commas(row), commas(dp::kResultCsvHeader));
    EXPECT_TRUE(row.starts_with("BTC-13NOV26-72000-C,call,72000,"));
}
