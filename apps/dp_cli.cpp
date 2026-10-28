// dp_cli: reads Deribit option quotes (CSV) from stdin and writes implied
// volatility and Greeks (CSV) to stdout. Diagnostics go to stderr.
//
//   python3 scripts/fetch_deribit.py | dp_cli --solver safeguarded > result.csv

#include "dp/quote_pricer.hpp"

#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr int kExitOk = 0;
constexpr int kExitNothingPriced = 1;
constexpr int kExitUsage = 2;

void printUsage(std::ostream& out)
{
    out << "Usage: dp_cli [--solver NAME] < quotes.csv > result.csv\n\n"
        << "Input columns:  " << dp::kQuoteCsvHeader << '\n'
        << "Output columns: " << dp::kResultCsvHeader << "\n\n"
        << "Solvers:";
    for (const std::string_view name : dp::solverNames()) {
        out << ' ' << name;
    }
    out << " (default: " << dp::solverNames().front() << ")\n";
}

}  // namespace

int main(int argc, char** argv)
{
    std::string_view solverName = dp::solverNames().front();
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            printUsage(std::cout);
            return kExitOk;
        }
        if (arg == "--solver" && i + 1 < argc) {
            solverName = argv[++i];
            continue;
        }
        std::cerr << "dp_cli: unexpected argument '" << arg << "'\n\n";
        printUsage(std::cerr);
        return kExitUsage;
    }

    const auto solver = dp::makeSolver(solverName);
    if (!solver) {
        std::cerr << "dp_cli: unknown solver '" << solverName << "'\n\n";
        printUsage(std::cerr);
        return kExitUsage;
    }

    std::ios::sync_with_stdio(false);
    std::cout << dp::kResultCsvHeader << '\n';

    std::string line;
    long lineNumber = 0;
    long priced = 0;
    long skipped = 0;
    while (std::getline(std::cin, line)) {
        ++lineNumber;
        if (line.empty() || line.starts_with("instrument_name")) {
            continue;
        }
        const auto quote = dp::parseQuoteCsvRow(line);
        if (!quote) {
            std::cerr << "line " << lineNumber << ": malformed row, skipped\n";
            ++skipped;
            continue;
        }
        const auto outcome = dp::priceQuote(*quote, *solver);
        if (const auto* error = std::get_if<dp::PricingError>(&outcome)) {
            std::cerr << "line " << lineNumber << " (" << quote->instrumentName << "): " << error->reason
                      << ", skipped\n";
            ++skipped;
            continue;
        }
        std::cout << dp::formatResultCsvRow(std::get<dp::PricingResult>(outcome)) << '\n';
        ++priced;
    }

    std::cerr << "dp_cli: priced " << priced << ", skipped " << skipped << " (solver: " << solver->name()
              << ")\n";
    return priced > 0 ? kExitOk : kExitNothingPriced;
}
