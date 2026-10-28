-- SQLite schema for dp_cli results (one row per priced option).
-- Loaded by scripts/load_results.py; columns mirror kResultCsvHeader in quote_pricer.hpp.

CREATE TABLE IF NOT EXISTS option_results (
    snapshot       TEXT    NOT NULL,  -- label of the market snapshot, e.g. '2026-10-28T12:00Z'
    instrument     TEXT    NOT NULL,
    option_type    TEXT    NOT NULL CHECK (option_type IN ('call', 'put')),
    strike         REAL    NOT NULL,
    expiry_years   REAL    NOT NULL,
    forward        REAL    NOT NULL,
    price_usd      REAL    NOT NULL,
    price_coin     REAL    NOT NULL,
    iv_model       REAL    NOT NULL,
    iv_deribit     REAL    NOT NULL,
    delta          REAL    NOT NULL,
    gamma          REAL    NOT NULL,
    vega           REAL    NOT NULL,
    theta          REAL    NOT NULL,
    iv_iterations  INTEGER NOT NULL,
    PRIMARY KEY (snapshot, instrument)
);

CREATE INDEX IF NOT EXISTS option_results_by_expiry ON option_results (snapshot, expiry_years, strike);
