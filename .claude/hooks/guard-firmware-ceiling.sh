#!/usr/bin/env bash
set -uo pipefail

payload="$(cat)"
tool="$(printf '%s' "$payload" | jq -r '.tool_name // empty' 2>/dev/null)"
[ -n "$tool" ] || exit 0

root="${CLAUDE_PROJECT_DIR:-$(printf '%s' "$payload" | jq -r '.cwd // empty' 2>/dev/null)}"
root="${root%/}"
[ -n "$root" ] || exit 0

ext='(c|h|s|S|ld|mk|cmake|cfg|gdb|ioc)'
names='(Makefile|makefile|GNUmakefile|CMakeLists\.txt|meson\.build|\.gdbinit)'
file_re="(^|/)(src/.+|${names}|[^/]+\.${ext})\$"

sep="(^|[[:space:]\"'=(])"
end="([[:space:]\"';&|)]|\$)"
token="(\./)?(src/[^[:space:]\"']*|[^/[:space:]\"'][^[:space:]\"']*\.${ext}|${names})"
token_re="${sep}${token}${end}"
redirect_re=">>?[[:space:]]*[\"']?${token}${end}"
writer_re="(^|[^[:alnum:]_])(tee|cp|mv|install|touch|truncate|rsync|ln|patch|dd)([[:space:]]|\$)|sed[[:space:]]+-[[:alpha:]]*i|perl[[:space:]]+-[[:alpha:]]*i|git[[:space:]]+apply|open[[:space:]]*\(|write_text|writeFile"

deny() {
  printf '%s\n' "FIRMWARE CEILING: $1. This repo is the author's hand-written firmware; AI explains, reviews and quizzes, it does not write src/, *.c, *.h, *.s, *.ld, Makefiles or debugger configs here. See CLAUDE.md. Do not work around this hook." >&2
  exit 2
}

case "$tool" in
  Write|Edit|MultiEdit|NotebookEdit)
    path="$(printf '%s' "$payload" | jq -r '.tool_input.file_path // .tool_input.notebook_path // empty' 2>/dev/null)"
    [ -n "$path" ] || exit 0
    case "$path" in
      "$root"/*) rel="${path#"$root"/}" ;;
      /*) exit 0 ;;
      *) rel="${path#./}" ;;
    esac
    [[ "$rel" =~ $file_re ]] && deny "$tool to '$rel' is blocked"
    ;;
  Bash)
    cmd="$(printf '%s' "$payload" | jq -r '.tool_input.command // empty' 2>/dev/null)"
    [ -n "$cmd" ] || exit 0
    cmd="${cmd//"$root"\//}"
    [[ "$cmd" =~ $redirect_re ]] && deny "shell redirect into a firmware path is blocked"
    if [[ "$cmd" =~ $writer_re ]] && [[ "$cmd" =~ $token_re ]]; then
      deny "shell write into a firmware path is blocked"
    fi
    ;;
esac

exit 0
