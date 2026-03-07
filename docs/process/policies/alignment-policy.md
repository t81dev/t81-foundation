# T81-AGI Alignment Policies

The Axion kernel supports explicit alignment policies to ensure high-tier cognitive agents remain within ethical and safety bounds.

## Alignment Requirements

Alignment policies are defined using the `require-alignment` clause in the Axion policy S-expression.

### Syntax

```lisp
(policy
  (tier 4)
  (require-alignment (reason "ethical-bound-01"))
  ...
)
```

## Enforcement

When an alignment requirement is present, the program MUST emit an Axion event containing the specified reason string before halting. Failure to do so results in a `SecurityFault` and a `Deny` verdict from the Axion kernel.

## Integrated Alignment Hooks

In T81Lang, you can trigger alignment events using effectful functions:

```t81
@effect
fn check_alignment() {
  write_axion_log(`alignment: ethical-bound-01 satisfied`);
}
```

Axion will scan the trace for these strings to verify compliance.
