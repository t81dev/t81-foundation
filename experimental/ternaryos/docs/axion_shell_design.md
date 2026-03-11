# Axion Shell Design Note

## Purpose

This note defines the intended direction of the Phase 5 Axion shell so future
implementation work has a stable target.

It is not a promise of full userland yet. It is the design reference for the
first real shell model on top of the hosted Axion storage/display path.

## Positioning

The Axion shell should not be:

- a Bash clone
- a PowerShell clone
- a GUI-first desktop launcher
- a generic REPL with no system identity

The Axion shell should be:

- coherent like a modern single-surface system shell
- inspectable like a Unix operator shell
- durable in a way common shells are not
- canon-native from the start

Working summary:

`coherent like macOS, inspectable like Linux, durable like Windows, but canon-native end to end`

## Naming Context

- `T81 Foundation` = umbrella project/ecosystem
- `T81VM` = ternary runtime/execution substrate
- `Axion` = operating system
- `CanonFS` and `TISC` remain subsystem names

The shell belongs to Axion. It is not the name for the entire T81 stack.

## Design Principles

1. One primary shell model

Axion should have one blessed shell interaction model instead of multiple
competing metaphors.

2. Text-first, object-aware

Commands should still be readable and scriptable as text, but the shell should
understand canonical objects, durable refs, and system state as first-class
things.

3. Durability is native

History, stored output, and recoverable session state should feel like part of
the shell contract, not an add-on.

4. System truth over illusion

If storage, messages, and audit identity are canon-native in Axion, the shell
should expose that directly rather than hiding behind fake Unix-like
abstractions.

5. Narrow before broad

The shell should become a minimal real shell first. Process spawning, general
userland, and syscall richness come later.

## Similarity To Existing Systems

### Similar To Apple

- one coherent primary surface
- strong defaults
- low visual clutter
- predictable interaction patterns

### Similar To Linux

- text remains a valid operator surface
- state is inspectable
- shell behavior should remain understandable and scriptable
- advanced users should not fight hidden magic

### Similar To Windows

- durable workflows matter
- the system should preserve meaningful history and state
- common actions should remain discoverable

## Where Axion Should Be Different

1. Canonical identity should be visible

The shell should be comfortable showing CanonRef handles, CanonFS object
identity, and durable state provenance.

2. History should be durable by design

Traditional shells treat history as a convenience file. Axion should treat
history as system state with explicit persistence semantics.

3. Storage and shell state should compose

`store put`, `history`, object inspection, and future IPC inspection should all
feel like one model rather than unrelated commands.

4. Deterministic replay should matter

The shell should be able to explain what happened, not just what is currently
on screen.

## User Model

The user should feel like they are working with:

- a current session
- a durable store
- canonical objects
- system/profile state

Not just an unstructured command stream.

The minimum mental model is:

- prompt: current action surface
- transcript: current session history
- store: durable object history
- profile: current machine/runtime shape

## Interaction Model

Axion shell should have three layers.

### Layer 1: Guided Built-ins

This is the current milestone.

- explicit built-in commands
- visible command list
- deterministic transcript
- durable `store put`
- reboot-aware `history`

### Layer 2: Typed Command Line

This should be the next milestone.

- freeform command input
- small parser
- command validation with clear errors
- no hidden modes

### Layer 3: Object-Native Shell

This is the longer-term target.

- commands operate on canonical objects, refs, and typed entities
- transcript can reference durable objects directly
- future IPC / process / TISC inspection fits the same grammar

## Screen Layout

The current TUI direction is correct and should remain the baseline.

Recommended persistent layout:

- transcript pane
- session/status pane
- command/help pane
- framebuffer or object preview pane

The shell should prefer one stable screen over many bouncing modes.

## Command Grammar

The shell grammar should stay intentionally small at first.

### Phase 5 Minimum Grammar

```text
help
profile
show profile
history
history show session
session status
session checkpoint
session export
session import <ref>
session diff <ref>
session run <ref>
name set <label> <ref>
name ls
object pin <kind> <name> <ref>
object ls
object show <name>
session show durable
show session
session refs
store put <text>
store put script <line>|<line>|...>
store put ref <ref>
store cp <ref>
store ls
store get <ref>
show ref <canonref>
store rm <ref>
history show object <ref>
history use <ref>
history show durable
clear
```

Rules:

- commands are lowercase canonical verbs
- subcommands are space-delimited
- errors should be explicit and short
- command output should be deterministic text

### Next Additions

```text
store cp <ref>
history show object <ref>
session export
session import <ref>
session diff <ref>
session run <ref>
history use <ref>
```

### Later Object-Native Direction

Current direction:

```text
show ref <canonref>
show session
show profile
session export
session import <ref>
session diff <ref>
session run <ref>
store put script "<cmd>|<cmd>"
store put text "<payload>"
store cp <ref>
store get ref <canonref>
show ref @label
object pin script bootstrap <ref>
session run @bootstrap
history show object <ref>
history use <ref>
```

This keeps the grammar readable while making objects explicit.

## Output Model

Output should be:

- concise by default
- structured when needed
- stable enough for snapshot testing

Axion should avoid command output that is only optimized for human scanning and
impossible for tools to reason about later.

Preferred pattern:

- short headline result
- optional structured detail
- canonical identifiers shown when relevant

## Distinctive Axion Qualities

The shell should feel recognizably Axion because it:

- treats durable history as part of the shell contract
- exposes CanonFS identity naturally
- keeps system profile visible
- makes reboot recovery a normal part of the story
- avoids pretending byte streams are the whole system

If Bash feels like a process launcher and PowerShell feels like an object
automation shell, Axion should feel like a canonical state shell.

## What To Avoid

- copying Unix syntax wholesale without system meaning
- inventing symbolic noise just to look novel
- building a fragile parser too early
- introducing multiple shell metaphors at once
- pretending processes/userland already exist when they do not

## Implementation Sequence

1. Replace menu-only command execution with typed input.
2. Keep the current built-ins and make them parse from a command line.
3. Add shell-focused tests for parse and transcript behavior.
4. Add `store ls` and `store get <ref>`.
5. Add `session status`, `session checkpoint`, `session refs`, `store rm <ref>`, `history show durable`, and `clear`.
6. Add session-local named refs so object-native commands can use stable aliases (`name set`, `name ls`, `@label`).
7. Add typed pinned objects so the shell can manage named object roles above raw aliases (`object pin`, `object ls`, `object show`).
8. Introduce quoted payload parsing and explicit parse errors.
9. Split shell-local transcript state from durable store state more explicitly.
10. Add the first object-native read commands: `show session` and `show ref <canonref>`.
11. Only then expand toward richer session/process/userland features.

## Acceptance For The Next Milestone

The next shell milestone should be considered complete when:

- the TUI accepts typed commands
- `help`, `profile`, `history`, and `store put <text>` work from typed input
- transcript updates deterministically
- persisted history still survives reboot through the existing Axion storage path
- snapshot mode remains stable for review/testing

This milestone is now complete. The next shell milestone should add:

- richer object composition and write grammar
- a clearer session-oriented history surface
- more direct profile/object inspection
- groundwork for a broader object-oriented command surface

## Summary

Axion shell should be a canonical state shell:

- one primary shell surface
- text-first but object-aware
- durable by design
- visually coherent
- honest about system structure

That is the right place to be similar to mainstream operating systems while
still being meaningfully different.
