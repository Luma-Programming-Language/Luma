# Rolling runtime image for the Luma compiler, published to GHCR by build.yml on
# every push to main. The playground (and anyone) can `FROM` it to get the
# latest `main` compiler.
#
# Build context must contain the `staging/` dir produced by the workflow:
#   staging/luma        the compiler binary
#   staging/*.so        bundled libLLVM + transitive deps ($ORIGIN rpath)
#   staging/std/        the standard library
#
# IMPORTANT: this base must match the glibc/libstdc++ of the runner that built
# `luma` (currently ubuntu-latest = Ubuntu 24.04). The binary requires GLIBC_2.38
# / GLIBCXX_3.4.32, which an older base (e.g. debian:bookworm) does not provide.
# If the build runner's Ubuntu version changes, bump this to match.
ARG RUNTIME_BASE=ubuntu:24.04
FROM ${RUNTIME_BASE}

# Base runtime libs the bundle intentionally does NOT vendor.
RUN apt-get update \
    && apt-get install -y --no-install-recommends libstdc++6 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Install the compiler + bundled libs + stdlib under one dir so $ORIGIN resolves.
COPY staging/ /usr/local/lib/luma/
RUN ln -sf /usr/local/lib/luma/luma /usr/local/bin/luma \
    && ldd /usr/local/lib/luma/luma \
    # Fail the build if any shared lib is unresolved.
    && ! ldd /usr/local/lib/luma/luma | grep -q 'not found'

# Default stdlib location (matches the playground backend default).
ENV LUMA_STD_DIR=/usr/local/lib/luma/std

CMD ["luma", "--help"]
