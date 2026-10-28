# Reproducible build and run in a clean Linux environment:
#   docker build -t deribit-pricer .
#   docker run --rm -i deribit-pricer < data/btc_options_sample.csv

FROM ubuntu:24.04 AS build
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ cmake make ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel \
    && ctest --test-dir build --output-on-failure

FROM ubuntu:24.04
COPY --from=build /src/build/apps/dp_cli /usr/local/bin/dp_cli
ENTRYPOINT ["dp_cli"]
