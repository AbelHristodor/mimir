#!/usr/bin/env sh
# Behaviour tests: feed lines to the shell, compare stdout.
#
#   shell.sh <binary>
set -u

bin=$1
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
fail=0

# check <name> <input> <expected stdout>
# The shell prints one '\n' on EOF, so expected output omits the final line.
check() {
  got=$(printf '%s\n' "$2" | "$bin" 2>/dev/null | sed '$d')
  if [ "$got" = "$3" ]; then
    echo "PASS  $1"
  else
    echo "FAIL  $1"
    echo "  input:    $2"
    echo "  expected: $(printf '%s' "$3" | tr '\n' '|')"
    echo "  got:      $(printf '%s' "$got" | tr '\n' '|')"
    fail=1
  fi
}

check "simple command"        'echo hi'                                'hi'
check "list status"           'false; echo x'                          'x'
check "pipe builtin to extern" 'echo hi | cat'                         'hi'
check "three stage pipe"      'printf b\na\n | sort | head -n 1'     'a'
check "pipe status is last"   'false | true && echo ok'                'ok'
check "negate pipeline"       '! true | false && echo ok'              'ok'
check "write and read file"   "echo x > $tmp/f; cat < $tmp/f"          'x'
check "append"                "echo x > $tmp/g; echo y >> $tmp/g; cat $tmp/g" 'x
y'
check "redirect mid command"  "echo a > $tmp/h b; cat $tmp/h"          'a b'
check "fd prefix redirect"    "ls /nonexistent 2> $tmp/e; cat $tmp/e | wc -l | xargs" '1'
check "digit stays a word"    'echo 2 | cat'                           '2'
check "no hang on empty stdin" 'cat | wc -c < /dev/null | xargs'  '0'
check "subshell in pipe"      'echo a | (cat; echo b)'                 'a
b'
check "subshell redirect"     "(echo s) > $tmp/s; echo n; cat $tmp/s"  'n
s'
check "syntax error aborts line" 'echo >; echo rc'                     ''
check "cd stays in parent"    "cd $tmp; pwd"                           "$(cd "$tmp" && pwd -P)"

exit $fail
