# check=skip=FromPlatformFlagConstDisallowed
# Reproducible build/test environment for Guard.
#
# Always x86-64: the attack tests assume the x86-64 frame layout (return
# address right above the saved frame pointer) and -fcf-protection only
# exists on x86. On Apple Silicon Docker emulates amd64 transparently.
FROM --platform=linux/amd64 ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
        build-essential gcc make gdb valgrind \
        python3 python3-venv python3-pip \
        ca-certificates git \
 && rm -rf /var/lib/apt/lists/*

# Python deps for the guardcc rewriter (Part 2), in a venv on PATH.
COPY rewriter/requirements.txt /tmp/requirements.txt
RUN python3 -m venv /opt/venv \
 && /opt/venv/bin/pip install --no-cache-dir -r /tmp/requirements.txt
ENV PATH="/opt/venv/bin:${PATH}"

# The repository is bind-mounted here at run time (see `make docker-test`).
WORKDIR /work
CMD ["bash"]
