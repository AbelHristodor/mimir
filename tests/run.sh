#!/usr/bin/env sh
# Trivial test runner. No framework.
#
#   run.sh ok    <binary>              pass if the program exits 0
#   run.sh catch <binary> <signature>  pass if the program aborts AND its output
#                                       contains <signature> (used for the
#                                       deliberately buggy sanitizer tests)
set -u

mode=$1
bin=$2
name=$(basename "$bin")

case "$mode" in
  ok)
    if "$bin"; then
      echo "PASS  $name (exit 0)"
    else
      code=$?
      echo "FAIL  $name (expected exit 0, got $code)"
      exit 1
    fi
    ;;
  catch)
    sig=$3
    out=$("$bin" 2>&1)
    code=$?
    if [ "$code" -ne 0 ] && printf '%s' "$out" | grep -q "$sig"; then
      echo "PASS  $name (caught: $sig)"
    else
      echo "FAIL  $name (expected abort matching '$sig', exit=$code)"
      printf '%s\n' "$out"
      exit 1
    fi
    ;;
  *)
    echo "usage: run.sh {ok|catch} <binary> [signature]" >&2
    exit 2
    ;;
esac
