-- Analytical queries over option_results. Run with:
--   sqlite3 results.db < sql/reports.sql

.headers on
.mode column

-- 1. Model validation: how far our implied volatility is from Deribit's mark_iv, per expiry.
SELECT
    ROUND(expiry_years * 365, 1)                            AS days_to_expiry,
    COUNT(*)                                                AS options,
    ROUND(AVG(ABS(iv_model - iv_deribit)) * 100, 4)         AS mean_abs_diff_vol_pts,
    ROUND(MAX(ABS(iv_model - iv_deribit)) * 100, 4)         AS max_abs_diff_vol_pts
FROM option_results
GROUP BY expiry_years
ORDER BY expiry_years;

-- 2. Volatility smile of the nearest expiry with more than a week left (out-of-the-money options only).
SELECT
    strike,
    option_type,
    ROUND(iv_model * 100, 2) AS iv_percent,
    ROUND(delta, 3)          AS delta
FROM option_results
WHERE expiry_years = (SELECT MIN(expiry_years) FROM option_results WHERE expiry_years > 7.0 / 365)
  AND ((option_type = 'call' AND strike >= forward) OR (option_type = 'put' AND strike < forward))
ORDER BY strike;
