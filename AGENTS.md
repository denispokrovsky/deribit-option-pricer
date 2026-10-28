# AGENTS.md

Instructions for AI coding agents working in this repository.

## Purpose

C++20 command-line tool that prices Deribit options with the Black-76 model: reads Deribit quotes
(CSV) from stdin, writes implied volatility and Greeks (CSV) to stdout. Python scripts fetch data
and load results into SQLite. Requirements: `docs/requirements.md`. Architecture and design
decisions: `docs/architecture.md`.

## Layout

- `include/dp/` — public headers of the `dp_core` library (documented with Doxygen comments).
- `src/` — library implementation; `src/text.hpp` is internal.
- `apps/dp_cli.cpp` — the CLI; keep it thin: parse args, read, call `priceQuote()`, write.
- `tests/` — GoogleTest unit tests and the CLI end-to-end test (`run_cli.cmake`).
- `bench/` — Google Benchmark; benchmark names are referenced from `docs/requirements.md`.
- `scripts/` (Python, standard library only), `sql/` (SQLite), `data/` (Deribit snapshot for tests).

## Commands

The target platform is Linux. The author's machine has no local C++ toolchain, so CI
(GitHub Actions) is the source of truth for build results.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDP_WARNINGS_AS_ERRORS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/bench/dp_bench --benchmark_min_time=0.2s
./build/apps/dp_cli < data/btc_options_sample.csv
docker build -t deribit-pricer .
```

## Code style

- C++20, standard library only in `dp_core`. Format with `.clang-format` (4-space indent, 120 columns).
- Names: `camelCase` functions and variables, `PascalCase` types, `kPascalCase` constants,
  `member_` for private data members, namespace `dp`.
- Prefer self-explanatory names (`timeToExpiry`, not `T`) over comments. Comment *why*, not *what*.
  Every public declaration in `include/dp/` has a Doxygen comment.
- The core (`black76`, `implied_vol`) does no I/O and knows nothing about Deribit.
- Expected failures (bad quote, expired option) are values (`std::optional`, `std::variant`,
  status enums); exceptions only for programming errors such as invalid model inputs.
- Python: standard library only, `argparse`, data on stdin/stdout, diagnostics on stderr.

## Rules for the agent

- Before finishing a task: build with warnings as errors and run all tests. If you cannot build
  locally, say so explicitly and rely on CI — never claim tests pass without seeing them pass.
- Every new behaviour gets a unit test; every bug fix gets a regression test.
- Numerical changes: verify against an independent reference (e.g. a short Python computation)
  and keep `QuotePricer.ReproducesDeribitMarkIv` passing.
- Do not change `.github/workflows/`, `Dockerfile` or dependency versions without an explicit request.
- Do not add third-party dependencies to `dp_core`.
- Do not commit, push or rewrite git history unless asked. Never commit secrets or API keys.
- When requirements or design change, update `docs/requirements.md` / `docs/architecture.md` too.
- Keep the README and this file in sync with the actual commands.
