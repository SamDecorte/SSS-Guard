CC      := gcc
WARN    := -Wall -Wextra
STD     := -std=gnu11
DEBUG   := -g -O0

# Flags that keep the attacks from being masked by other mitigations.
# Do NOT use these for libguard itself.
ATTACK_HARDEN := -fno-stack-protector -no-pie -fcf-protection=none -rdynamic

# Flags the shadow stack relies on.
SS_FLAGS := -fno-omit-frame-pointer -fno-optimize-sibling-calls -falign-functions=16

# Extra flags from the command line, e.g. `make EXTRA_CFLAGS=-Werror test`.
EXTRA_CFLAGS ?=
CFLAGS  := $(STD) $(WARN) $(DEBUG) $(SS_FLAGS) $(EXTRA_CFLAGS)
LDLIBS  := -ldl

BUILD   := build
LIB     := $(BUILD)/guard.o

# Benign tests: exit 0 and same stdout with and without Guard.
BENIGN  := callbacks api_demo fork_icall icall_once ss_nested
# Detect tests: guarded build must stop with a "guard: ..." error.
DETECT  := fptr_midfunc deep_recursion register_after_seal
# Attack tests: hijack when plain, blocked when guarded.
ATTACK  := fptr_overwrite ret_overwrite fptr_libc ret_to_registered

# run_tests.sh reads these, so the lists live in one place.
export BENIGN DETECT ATTACK

.PHONY: all test clean
all: $(addprefix $(BUILD)/,$(BENIGN) $(DETECT)) \
     $(addprefix $(BUILD)/,$(addsuffix .plain,$(BENIGN) $(ATTACK))) \
     $(addprefix $(BUILD)/,$(addsuffix .guard,$(ATTACK)))

$(BUILD):
	mkdir -p $(BUILD)

$(LIB): libguard/guard.c libguard/guard.h | $(BUILD)
	$(CC) $(CFLAGS) -Ilibguard -c $< -o $@

# Benign (guarded) test binaries.
$(BUILD)/%: testcases/%.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(ATTACK_HARDEN) -Ilibguard -Itestcases $< $(LIB) -o $@ $(LDLIBS)

# Attack binaries, plain: macros disabled, no libguard logic -> hijack.
$(BUILD)/%.plain: testcases/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(ATTACK_HARDEN) -DGUARD_DISABLE -Ilibguard -Itestcases \
	    $< libguard/guard.c -o $@ $(LDLIBS)

# Attack binaries, guarded.
$(BUILD)/%.guard: testcases/%.c $(LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(ATTACK_HARDEN) -Ilibguard -Itestcases $< $(LIB) -o $@ $(LDLIBS)

test: all
	@bash ./run_tests.sh

clean:
	rm -rf $(BUILD) *.guard.c

# ------------------------------------------------------------------
# Docker: identical x86-64 Linux toolchain for everyone (and for CI).
#   make docker-test    build the image, then clean + test inside it
#   make docker-shell   interactive shell in the container
# ------------------------------------------------------------------
IMAGE      := sss-guard
DOCKER_RUN := docker run --rm --platform linux/amd64 -v "$(CURDIR)":/work -w /work

.PHONY: docker-image docker-test docker-shell
docker-image:
	docker build --platform linux/amd64 -t $(IMAGE) .

docker-test: docker-image
	$(DOCKER_RUN) $(IMAGE) sh -c 'make clean && make test EXTRA_CFLAGS="$(EXTRA_CFLAGS)"'

docker-shell: docker-image
	$(DOCKER_RUN) -it $(IMAGE) bash
