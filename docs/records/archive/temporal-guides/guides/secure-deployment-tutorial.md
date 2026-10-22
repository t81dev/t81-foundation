# Tutorial: Building and Deploying Secure Ternary Applications

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Tutorial: Building and Deploying Secure Ternary Applications](#tutorial-building-and-deploying-secure-ternary-applications)
  - [Step 1: Write the T81Lang Source](#step-1-write-the-t81lang-source)
  - [Step 2: Author the Axion Policy](#step-2-author-the-axion-policy)
  - [Step 3: Compile and Validate](#step-3-compile-and-validate)
  - [Step 4: Secure Execution](#step-4-secure-execution)
  - [Step 5: Auditing the Trace](#step-5-auditing-the-trace)
  - [Conclusion](#conclusion)

<!-- T81-TOC:END -->


This tutorial provides a step-by-step guide to developing, securing, and auditing an application on the T81 Foundation stack. We will build a simple "Secure Vault" that uses Axion policies to ensure all memory access is audited.

______________________________________________________________________

## Step 1: Write the T81Lang Source

Create a file named `vault.t81`:

```t81
// vault.t81
// A simple secure vault that requires audited access

enum AccessResult {
    Granted(T81Int),
    Denied
}

fn check_vault(key: T81Int) -> AccessResult {
    if (key == 1337) {
        return AccessResult.Granted(81);
    } else {
        return AccessResult.Denied;
    }
}

let attempt = check_vault(1337);

let secret = match (attempt) {
    AccessResult.Granted(val) => val,
    AccessResult.Denied => 0,
};
```

______________________________________________________________________

## Step 2: Author the Axion Policy

To ensure this program runs securely, we want to enforce that the `match` expression over `AccessResult` is logged and that memory segments are correctly allocated.

Create `vault.axion`:

```lisp
(policy
  (tier 1)
  (require-match-guard
    (enum AccessResult)
    (variant Granted)
    (payload T81Int)
    (result pass))
  (require-segment-event
    (segment stack)
    (action "stack frame allocated")))
```

______________________________________________________________________

## Step 3: Compile and Validate

First, validate that your policy file is syntactically correct:

```bash
python3 tools/axion_policy_validator.py vault.axion
```

Now, compile the application with the policy embedded:

```bash
./build/t81 compile vault.t81 --axion-policy vault.axion -o vault.tisc
```

______________________________________________________________________

## Step 4: Secure Execution

Run the compiled program. HanoiVM will automatically engage the Axion kernel to enforce the embedded policy.

```bash
./build/t81 run vault.tisc
```

If the program completes normally, it means all Axion predicates were satisfied. If you change the code to bypass the `match` (e.g., by using an unsafe unwrap if one existed), Axion would trigger a `SecurityFault`.

______________________________________________________________________

## Step 5: Auditing the Trace

To provide proof to an auditor, capture the deterministic Axion trace:

```bash
./build/t81 run --axion-log vault.tisc > vault_trace.log
```

You can then use the `tools/axion_validator.py` (if running in an offline environment) to re-verify the log against the policy.

______________________________________________________________________

## Conclusion

By following this workflow, you ensure that your T81 applications are not only deterministic but also compliant with verifiable safety policies. This "Defense-in-Depth" approach is what makes the T81 Foundation suitable for high-stakes cognitive computing.
