# Project

Source-level control-flow integrity for C. See the assignment for the full specification.
This repository is a skeleton: `libguard` is stubbed and the test cases are instrumented by hand with the `GUARD_*` macros.

## Layout

- `libguard/`    the runtime you implement (`guard.c`, `guard.h`)
- `testcases/`   benign and attack programs
- `rewriter/`    rewriter skeleton code
- `Makefile`     builds guarded and plain binaries
- `run_tests.sh` the test runner invoked by `make test`

## Build and test

```sh
make          # build libguard and all test binaries
make test     # build, then run the suite
```

The code only builds on x86-64 Linux. On macOS (including Apple Silicon) or
Windows, use the Docker image, which is also what CI runs:

```sh
make docker-test                    # build the image, then `make clean test` inside it
make docker-test EXTRA_CFLAGS=-Werror   # what CI runs
make docker-shell                   # shell in the container (gcc, gdb, valgrind, guardcc deps)
```

CI (`.github/workflows/ci.yml`) runs `make docker-test EXTRA_CFLAGS=-Werror` on
every push to `main` and on every pull request.

### Adding a test

Put the `.c` file in `testcases/` and add its name to exactly one list in the `Makefile`:

- `BENIGN`: must exit 0 and print the same stdout as its `-DGUARD_DISABLE` build.
- `DETECT`: the guarded build must stop with a `guard: ...` message on stderr.
- `ATTACK`: `.plain` must hijack (exit 42), `.guard` must stop with a `guard: ...` message.

## What "pass" means

- Benign tests must exit 0 and print the same output with and without Guard.
- Detect tests must be stopped by Guard with a clear `guard:` error (not a random crash).
- Attack tests are built twice. The `.plain` build (`-DGUARD_DISABLE`) must hijack: `win()` runs and the process exits 42.
  The `.guard` build must detect the hijack and terminate before `win()` runs.

Out of the box the stubs do nothing, so the guarded attack builds still hijack and `make test` reports failures.
Your job is to make them pass.

## Notes

- `win()` is never called directly or address-taken; the attacker finds
  it with `dlsym` (hence `-rdynamic`). Guard must never register it.
- Attack binaries use `-fno-stack-protector -no-pie -fcf-protection=none`
  so the effect of *your* protection is not hidden by other mitigations.
- The shadow stack needs `-fno-omit-frame-pointer`, `-fno-optimize-sibling-calls` and `-falign-functions=16`.
