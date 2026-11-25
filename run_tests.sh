#!/usr/bin/env bash
# Test runner for `make test`. The test lists (BENIGN, DETECT, ATTACK)
# come from the Makefile so there is only one place to add a test.
set -u
B=build
HIJACK=42
: "${BENIGN:?run via 'make test'}" "${DETECT:?run via 'make test'}" "${ATTACK:?run via 'make test'}"

# Kill hanging tests instead of hanging CI (timeout is not on macOS).
T=""; command -v timeout >/dev/null && T="timeout 10"

pass=0; fail=0
ok(){ echo "PASS $1"; pass=$((pass+1)); }
no(){ echo "FAIL $1 : $2"; fail=$((fail+1)); }
show(){ [ -n "$2" ] && printf '%s\n' "$2" | sed "s/^/    [$1] /"; return 0; }

# run <binary>: sets $out (stdout), $err (stderr), $code (exit status).
run(){
  local e; e=$(mktemp)
  out=$($T "$1" 2>"$e"); code=$?
  err=$(cat "$e"); rm -f "$e"
}

# --- benign: exit 0, and same stdout as the unprotected build ---
for t in $BENIGN; do
  echo "-- $t --"
  run "$B/$t.plain"; pout=$out
  run "$B/$t"
  show out "$out"; show err "$err"
  if [ "$code" -ne 0 ]; then no "$t (benign)" "guarded exited $code"
  elif [ "$out" != "$pout" ]; then
    no "$t (benign)" "output differs from plain build"
    diff <(printf '%s\n' "$pout") <(printf '%s\n' "$out") | sed 's/^/    /'
  else ok "$t (benign)"; fi
done

# --- detect: guarded build must stop with a clear Guard error ---
for t in $DETECT; do
  echo "-- $t --"
  run "$B/$t"
  show out "$out"; show err "$err"
  if [ "$code" -eq 0 ]; then no "$t (detect)" "exited 0, nothing detected"
  elif ! grep -q "guard:" <<<"$err"; then
    no "$t (detect)" "exited $code without a 'guard:' message (crash, not detection?)"
  else ok "$t (detect)"; fi
done

# --- attacks: plain hijacks, guarded blocks before win() ---
for t in $ATTACK; do
  echo "-- $t --"
  run "$B/$t.plain"; p=$code
  show plain "$out"
  run "$B/$t.guard"; g=$code
  show guard "$out"; show guard "$err"
  if [ "$p" -ne "$HIJACK" ]; then no "$t" "plain did not hijack (exit $p, expected $HIJACK)"
  elif [ "$g" -eq "$HIJACK" ] || [ "$g" -eq 0 ]; then no "$t" "guarded was NOT blocked (exit $g)"
  elif ! grep -q "guard:" <<<"$err"; then
    no "$t" "guarded exited $g without a 'guard:' message (crash, not detection?)"
  else ok "$t (plain hijacks, guarded blocks)"; fi
done

echo "-----"
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
