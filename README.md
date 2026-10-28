# Deribit Option Pricer

[![CI](https://github.com/denispokrovsky/deribit-option-pricer/actions/workflows/ci.yml/badge.svg)](https://github.com/denispokrovsky/deribit-option-pricer/actions/workflows/ci.yml)
[![Documentation](https://github.com/denispokrovsky/deribit-option-pricer/actions/workflows/docs.yml/badge.svg)](https://denispokrovsky.github.io/deribit-option-pricer/)

Консольная программа на C++20, которая считает **подразумеваемую волатильность и греки опционов
биржи Deribit** по модели Black-76 — так же, как это делает сама биржа.

📖 **Документация:** <https://denispokrovsky.github.io/deribit-option-pricer/>
· [Требования](https://denispokrovsky.github.io/deribit-option-pricer/requirements.html)
· [Архитектура](https://denispokrovsky.github.io/deribit-option-pricer/architecture.html)
(исходники: `docs/requirements.md`, `docs/architecture.md`)

## Что делает программа

На вход (stdin) — котировки опционов Deribit в CSV, на выход (stdout) — для каждого опциона:
цена в USD и в BTC, наша IV и IV биржи (`mark_iv`) для сравнения, delta, gamma, vega, theta.

```text
$ ./build/apps/dp_cli < data/btc_options_sample.csv
instrument_name,type,strike,expiry_years,forward,price_usd,price_coin,iv_model,iv_deribit,delta,gamma,vega,theta,iv_iterations
BTC-13NOV26-72000-C,call,72000,0.043036,80733.66,...
```

Вокруг C++-ядра — небольшой конвейер:

```bash
python3 scripts/fetch_deribit.py --currency BTC > quotes.csv                 # свежие котировки (без API-ключа)
./build/apps/dp_cli < quotes.csv > result.csv                                 # IV и греки
python3 scripts/load_results.py --db results.db --snapshot now < result.csv  # в SQLite
sqlite3 results.db < sql/reports.sql                                          # сверка с Deribit, улыбка
```

## Сборка и запуск

Нужны Linux, CMake ≥ 3.25 и компилятор с поддержкой C++20 (GCC 13+ или Clang 18+).
GoogleTest и Google Benchmark CMake скачает сам.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure          # тесты
./build/bench/dp_bench                              # бенчмарк
./build/apps/dp_cli --help
```

Без установки чего-либо, кроме Docker:

```bash
docker build -t deribit-pricer .
docker run --rm -i deribit-pricer < data/btc_options_sample.csv
```

## Структура

| Папка | Содержимое |
|-------|-----------|
| `include/dp/`, `src/` | Библиотека `dp_core`: модель Black-76, решатели IV, адаптеры Deribit |
| `apps/` | Консольная программа `dp_cli` |
| `tests/` | Модульные тесты (GoogleTest) и сквозной тест CLI |
| `bench/` | Бенчмарк (Google Benchmark) |
| `scripts/` | Python: загрузка котировок Deribit, загрузка результатов в SQLite |
| `sql/` | Схема БД и аналитические запросы |
| `data/` | Снимок реальных котировок BTC-опционов Deribit для тестов |
| `docs/` | Требования и архитектура (входят в документацию Doxygen) |

## CI/CD

При каждом push GitHub Actions:

1. собирает проект GCC и Clang (предупреждения = ошибки);
2. запускает тесты, включая сквозной прогон CLI на реальных данных Deribit;
3. запускает бенчмарк — результаты видны в журнале шага **Benchmark**;
4. загружает результат в SQLite и выполняет SQL-отчёты;
5. собирает и запускает Docker-образ в чистом окружении;
6. при push в `main` — генерирует документацию Doxygen и публикует её на GitHub Pages.

## Разработка с ИИ

Проект разрабатывается с ИИ-агентом Claude Code. Правила для агента — в [AGENTS.md](AGENTS.md)
(Claude Code подключает его через [CLAUDE.md](CLAUDE.md)).
