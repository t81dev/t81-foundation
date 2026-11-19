#include "tools/cli/core/cli_docs_gen.hpp"

namespace t81::cli {

std::string build_bash_completion() {
  return R"(_t81_complete() {
  local cur prev words cword
  _init_completion || return
  local commands="code lang project env internal canonfs determinism vm tisc ir tier tensor ai weights model policy axion trace mcp c rust python llvm mlir repl studio agent ui completion man feedback version help"
  if [[ ${cword} -eq 1 ]]; then
    COMPREPLY=( $(compgen -W "${commands}" -- "${cur}") )
    return
  fi
  case "${words[1]}" in
    lang)
      COMPREPLY=( $(compgen -W "check lint fmt build run test disasm debug repl show dump export validate profile" -- "${cur}") )
      ;;
    code)
      COMPREPLY=( $(compgen -W "check lint fmt build run test disasm debug repl profile" -- "${cur}") )
      ;;
    project)
      COMPREPLY=( $(compgen -W "init build run test" -- "${cur}") )
      ;;
    env)
      COMPREPLY=( $(compgen -W "check doctor paths diag toolchain clean feedback" -- "${cur}") )
      ;;
    internal)
      COMPREPLY=( $(compgen -W "pkg benchmark repro-hash canonize-tensor canonize-file memory-stats llama-run" -- "${cur}") )
      ;;
    canonfs)
      COMPREPLY=( $(compgen -W "put-file put-tensor ls get stat verify snapshot snapshot-diff rollback gc fsck repair" -- "${cur}") )
      ;;
    determinism)
      COMPREPLY=( $(compgen -W "verify verify-run compare-run certify explain hash trace-hash diff diff-trace baseline bisect multi-run" -- "${cur}") )
      ;;
    vm)
      COMPREPLY=( $(compgen -W "run debug trace until step regs stack mem state profile explain-trap" -- "${cur}") )
      ;;
    tisc)
      COMPREPLY=( $(compgen -W "disasm validate stats encode decode diff" -- "${cur}") )
    ;;
    ir)
      COMPREPLY=( $(compgen -W "show dump export validate" -- "${cur}") )
    ;;
    tier)
      COMPREPLY=( $(compgen -W "info check gate" -- "${cur}") )
    ;;
    tensor)
      COMPREPLY=( $(compgen -W "canonize hash inspect" -- "${cur}") )
      ;;
    ai)
      COMPREPLY=( $(compgen -W "backend model verify inference quantization benchmark policy workflow observability run quantize" -- "${cur}") )
      ;;
    weights)
      COMPREPLY=( $(compgen -W "import info verify export quantize" -- "${cur}") )
      ;;
    policy)
      COMPREPLY=( $(compgen -W "compile validate run test list" -- "${cur}") )
      ;;
    axion)
      COMPREPLY=( $(compgen -W "status optimize simulate explain snapshot snapshot-diff rollback log audit" -- "${cur}") )
      ;;
    trace)
      COMPREPLY=( $(compgen -W "show diff replay summary stats filter canonicalize export" -- "${cur}") )
      ;;
    mcp)
      COMPREPLY=( $(compgen -W "serve help" -- "${cur}") )
      ;;
    c)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    rust)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    python)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    llvm)
      COMPREPLY=( $(compgen -W "compile help" -- "${cur}") )
      ;;
    mlir)
      COMPREPLY=( $(compgen -W "compile lower pipeline help" -- "${cur}") )
      ;;
    completion)
      COMPREPLY=( $(compgen -W "bash zsh fish" -- "${cur}") )
      ;;
    feedback)
      COMPREPLY=( $(compgen -W "submit report" -- "${cur}") )
      ;;
  esac
}
complete -F _t81_complete t81
)";
}

std::string build_zsh_completion() {
  return R"(#compdef t81
local context state line
typeset -A opt_args

local -a commands
commands=(
  'lang:T81Lang-oriented workflow commands'
  'code:code workflow commands'
  'project:project lifecycle commands'
  'env:environment commands'
  'internal:internal commands'
  'canonfs:CanonFS inspection and snapshot commands'
  'determinism:determinism verification and hashing commands'
  'vm:VM execution and state inspection'
  'tisc:TISC artifact inspection and validation'
  'ir:IR inspection for frontend lowering'
  'tier:cognitive tier inspection and gating'
  'tensor:tensor artifact tools'
  'ai:Bounded AI tasks, model inspection, and partial inference tools'
  'weights:model weight tools'
  'policy:Axion policy tools'
  'axion:Axion governor tools'
  'trace:trace inspection tools'
  'mcp:native C++ JSON-RPC 2.0 MCP server (15 tools)'
  'c:Experimental C-subset frontend to MLIR'
  'rust:Experimental Rust-subset frontend to MLIR'
  'python:Experimental Python-subset frontend to MLIR'
  'llvm:LLVM IR backend — translate TISC to LLVM IR/bitcode'
  'mlir:MLIR frontend — translate TISC to MLIR or LLVM IR'
  'studio:human operator TUI'
  'agent:AI-native TUI'
  'ui:interactive TUI launcher'
  'completion:print completion script'
  'man:show or install man page'
  'feedback:local CLI feedback loop'
  'help:show help'
  'version:show version'
)

_arguments -C \
  '1:command:->command' \
  '*::arg:->args'

case $state in
  command)
    _describe -t commands 't81 command' commands
    ;;
  args)
    case $words[2] in
      lang)
        _values 'lang action' check lint fmt build run test disasm debug repl show dump export validate profile
        ;;
      code)
        _values 'code action' check lint fmt build run test disasm debug repl profile
        ;;
      project)
        _values 'project action' init build run test
        ;;
      env)
        _values 'env action' check doctor paths diag toolchain clean feedback
        ;;
      internal)
        _values 'internal action' pkg benchmark repro-hash canonize-tensor canonize-file memory-stats llama-run
        ;;
      canonfs)
        _values 'canonfs action' put-file put-tensor ls get stat verify snapshot snapshot-diff rollback gc fsck repair
        ;;
      determinism)
        _values 'determinism action' verify verify-run compare-run certify explain hash trace-hash diff diff-trace baseline bisect multi-run
        ;;
      vm)
        _values 'vm action' run debug trace until step regs stack mem state profile explain-trap
        ;;
      tisc)
        _values 'tisc action' disasm validate stats encode decode diff
        ;;
      ir)
        _values 'ir action' show dump export validate
        ;;
      tier)
        _values 'tier action' info check gate
        ;;
      tensor)
        _values 'tensor action' canonize hash inspect
        ;;
      ai)
        _values 'ai action' backend model verify inference quantization benchmark policy workflow observability run quantize
        ;;
      weights)
        _values 'weights action' import info verify export quantize
        ;;
      policy)
        _values 'policy action' compile validate run test list
        ;;
      axion)
        _values 'axion action' status optimize simulate explain snapshot snapshot-diff rollback log audit
        ;;
      trace)
        _values 'trace action' show diff replay summary stats filter canonicalize export
        ;;
      mcp)
        _values 'mcp action' serve help
        ;;
      c)
        _values 'c action' compile help
        ;;
      rust)
        _values 'rust action' compile help
        ;;
      python)
        _values 'python action' compile help
        ;;
      llvm)
        _values 'llvm action' compile help
        ;;
      mlir)
        _values 'mlir action' compile lower pipeline help
        ;;
      feedback)
        _values 'feedback action' submit report
        ;;
      completion)
        _values 'shell' bash zsh fish
        ;;
    esac
    ;;
esac
)";
}

std::string build_fish_completion() {
  return R"(complete -c t81 -f -n '__fish_use_subcommand' -a 'code lang project env internal canonfs determinism vm tisc ir tier tensor ai weights model policy axion trace mcp c rust python llvm mlir repl studio agent ui completion man feedback version help'
complete -c t81 -f -n '__fish_seen_subcommand_from completion' -a 'bash zsh fish'
complete -c t81 -f -n '__fish_seen_subcommand_from lang' -a 'check lint fmt build run test disasm debug repl show dump export validate profile'
complete -c t81 -f -n '__fish_seen_subcommand_from code' -a 'check lint fmt build run test disasm debug repl profile'
complete -c t81 -f -n '__fish_seen_subcommand_from project' -a 'init build run test'
complete -c t81 -f -n '__fish_seen_subcommand_from env' -a 'check doctor paths diag toolchain clean feedback'
complete -c t81 -f -n '__fish_seen_subcommand_from internal' -a 'pkg benchmark repro-hash canonize-tensor canonize-file memory-stats llama-run'
complete -c t81 -f -n '__fish_seen_subcommand_from canonfs' -a 'put-file put-tensor ls get stat verify snapshot snapshot-diff rollback gc fsck repair'
complete -c t81 -f -n '__fish_seen_subcommand_from determinism' -a 'verify verify-run compare-run certify explain hash trace-hash diff diff-trace baseline bisect multi-run'
complete -c t81 -f -n '__fish_seen_subcommand_from vm' -a 'run debug trace until step regs stack mem state profile explain-trap'
complete -c t81 -f -n '__fish_seen_subcommand_from tisc' -a 'disasm validate stats encode decode diff'
complete -c t81 -f -n '__fish_seen_subcommand_from ir' -a 'show dump export validate'
complete -c t81 -f -n '__fish_seen_subcommand_from tier' -a 'info check gate'
complete -c t81 -f -n '__fish_seen_subcommand_from tensor' -a 'canonize hash inspect'
complete -c t81 -f -n '__fish_seen_subcommand_from ai' -a 'backend model verify inference quantization benchmark policy workflow observability run quantize'
complete -c t81 -f -n '__fish_seen_subcommand_from weights' -a 'import info verify export quantize'
complete -c t81 -f -n '__fish_seen_subcommand_from policy' -a 'compile validate run test list'
complete -c t81 -f -n '__fish_seen_subcommand_from axion' -a 'status optimize simulate explain snapshot snapshot-diff rollback log audit'
complete -c t81 -f -n '__fish_seen_subcommand_from trace' -a 'show diff replay summary stats filter canonicalize export'
complete -c t81 -f -n '__fish_seen_subcommand_from mcp' -a 'serve help'
complete -c t81 -f -n '__fish_seen_subcommand_from c' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from rust' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from python' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from llvm' -a 'compile help'
complete -c t81 -f -n '__fish_seen_subcommand_from mlir' -a 'compile lower pipeline help'
complete -c t81 -f -n '__fish_seen_subcommand_from feedback' -a 'submit report'
)";
}

std::string build_manpage_text() {
  return R"(.TH T81 1 "2026-02-26" "t81 CLI" "User Commands"
.SH NAME
t81 \- T81 Foundation CLI
.SH SYNOPSIS
t81 [global-options] <command> [args]
t81 [global-options] <domain> <action> [args]
.SH DESCRIPTION
Domain-first commands:
.TP
lang
T81Lang-oriented workflow for source compile/check/run and IR lowering.
.TP
code
Check, build, run, test, format, debug, disassemble, repl.
.TP
canonfs
Put, inspect, verify, snapshot, diff, and roll back CanonFS objects.
.TP
determinism
Hash artifacts, diff traces, and verify repeatable execution.
.TP
vm
Run, trace, debug, profile, and inspect TISC execution.
.TP
tisc
Disassemble, validate, encode, and decode TISC artifacts.
.TP
ir
Show and export lowered IR from T81 source.
.TP
tensor
Canonicalize, hash, and inspect tensor artifacts.
.TP
weights
Import, inspect, verify, and quantize model weights.
.TP
policy
Compile, run, and test Axion policies.
.TP
axion
Inspect governor state, simulate, snapshot, diff snapshots, and roll back.
.TP
trace
Show, diff, replay, canonicalize, and export execution traces.
.TP
mcp
Run the native C++ JSON-RPC 2.0 MCP server (`t81 mcp serve`, 15 tools).
.TP
project
Project lifecycle commands.
.TP
env
Environment checks and UX feedback.
.TP
internal
Internal and experimental operations.
.SH GLOBAL OPTIONS
-h, --help
-V, --version
-q, --quiet
-v, --verbose
.SH SEE ALSO
t81 help, t81 help code, t81 help advanced, t81 help labs
)";
}

}  // namespace t81::cli
