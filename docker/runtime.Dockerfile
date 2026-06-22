ARG RUNTIME_BASE=ubuntu:24.04
FROM ${RUNTIME_BASE}

RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY staging/ /usr/local/lib/luma/
RUN ln -sf /usr/local/lib/luma/luma /usr/local/bin/luma \
    && ln -sf /usr/local/lib/luma/clang /usr/local/bin/clang \
    && ln -sf /usr/local/lib/luma/clang /usr/local/bin/cc \
    && ldd /usr/local/lib/luma/luma \
    && ! ldd /usr/local/lib/luma/luma | grep -q 'not found'

ENV LUMA_STD_DIR=/usr/local/lib/luma/std \
    CC=clang

CMD ["luma", "--help"]
