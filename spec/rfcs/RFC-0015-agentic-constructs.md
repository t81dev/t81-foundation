______________________________________________________________________

title: "RFC-0015 — First-Class Agents and Tiered Recursive Cognition"
version: Draft
applies_to:

- T81Lang Specification
- TISC Specification
- Axion Kernel Specification

______________________________________________________________________

# Summary

This RFC proposes the introduction of a new top-level declaration, `agent`, to T81Lang. An `agent` is a construct for encapsulating state (e.g., neural network weights) and behaviors (e.g., `infer` or `train` methods) into a single, cohesive unit. This provides the language with a powerful abstraction for building complex, stateful, and recursive AI systems, with deep integration into Axion's cognitive-tier safety model.

# Motivation

The previous RFCs (`0012`-`0014`) introduced the *primitives* for AI operations. However, there is no language-level construct to organize these primitives into a coherent entity. A neural network is more than just weights and functions; it's a model of behavior. The `agent` construct is needed to:

1.  **Encapsulate State and Behavior:** An `agent` binds data (like `TernaryTensor` weights) to the functions that operate on that data. This is a fundamental principle of structured design and is crucial for managing the complexity of large AI models.
2.  **Enable Tiered Cognition:** Agents are the primary unit of cognitive identity that Axion reasons about. Declaring an `agent` allows developers to assign it a cognitive tier, enabling Axion to enforce policies on its recursion depth, resource usage, and interaction with other agents.
3.  **Provide a Namespace for Behaviors:** An `agent` provides a natural namespace for its behaviors (e.g., `SimpleNet.infer(...)`), improving code organization and readability.
4.  **Formalize Recursive Self-Calls:** The `agent` model provides a formal basis for recursive cognition. An agent's behavior can call another agent, or even itself, with Axion acting as a supervisor to prevent uncontrolled recursion.

# Guide-level explanation

This RFC introduces a new way to organize your AI code: the `agent`. You can think of an `agent` as a container for a neural network's data and its abilities.

Instead of having loose functions and variables, you can group them together. An agent has a name and a set of named `behaviors`, which are like methods.

Here is the example from the original proposal, which now becomes possible with this RFC:

```t81
agent SimpleNet {
  // This is a behavior named "infer"
  @neural(layer=2) @tier(3)
  behavior infer(input: TernaryTensor[Trit, 784]) -> TernaryTensor[Trit, 10] {
    let weights = quantize([0.5t81, -0.3t81, ...] as TernaryTensor[Trit, 784, 10]);
    return input ** weights;  // Ternary matmul
  }
}

fn main() {
  let img = tensor[1t81, 0t81, ...];
  // You call an agent's behavior like this:
  let output = SimpleNet.infer(img);
  print(quantize(output));
}
```

This makes the code much more organized and allows the `Axion` kernel to apply safety rules at the agent level.

# Reference-level implementation

## 1. Specification Changes

The following changes would be made to the `t81lang-spec.md` upon acceptance of this RFC.

### A. Grammar (`§1 Core Grammar`)

The `program` and `factor` productions will be updated, and new `agent` and `behavior` productions will be added.

```ebnf
program       ::= { function | agent | declaration }*  // New: agent

agent         ::= "agent" identifier "{" behavior* "}"
behavior      ::= "behavior" identifier "(" parameters ")" "->" type block

factor        ::= literal
                | identifier
                | fn_call
                | agent_call     // New
                | ... (omitted)

agent_call    ::= identifier "." identifier "(" [ expr { "," expr } ] ")"
```

The `agent_call` syntax provides a clear and unambiguous way to invoke a specific behavior on an agent.

### B. Compilation Pipeline (`§5 Compilation Pipeline`)

#### Name Resolution (`§4`)

-   Agents introduce a new top-level scope. Agent names are resolved at the module level.
-   Behavior names are resolved within the scope of their agent (e.g., the `infer` in `SimpleNet.infer`).

#### TISC Lowering (`§5.7 TISC Lowering`)

A new row will be added to the lowering table:

| IR Construct | TISC Output |
| --- | --- |
| Agent behavior call | `AGENT_INVOKE`, tier-checked recursion |

The `AGENT_INVOKE` opcode is a new TISC instruction that is a specialized version of `CALL`. It includes metadata about the agent and behavior being invoked, which is visible to Axion for tier-checking and policy enforcement.

### C. Axion Integration (`§7 Axion Integration`)

-   Agents become a primary subject for Axion policies. Axion can observe `AGENT_INVOKE` calls to:
    -   Veto calls that would exceed the maximum recursion depth for a given cognitive tier.
    -   Monitor resource usage on a per-agent basis.
    -   Enforce information flow policies between agents.

# Drawbacks

-   The introduction of `agent` moves T81Lang slightly towards an object-oriented paradigm. This adds complexity compared to a purely procedural language. However, this is a justified trade-off for the massive benefits in organization and safety for AI applications.

# Rationale and alternatives

-   **Why not use modules or records?** While modules provide namespacing and records group data, neither provides the combination of bundled state, behavior, and first-class cognitive identity that is required for Axion integration. An `agent` is a unique construct specifically for this purpose.

# Future Possibilities

-   This RFC completes the core set of AI-native language features from the original "Version 0.3" proposal.
-   Future RFCs could explore dynamic agent creation, inter-agent communication protocols, and more advanced Axion policies for governing agent societies.

# Open Questions

1.  Will agents have internal state (i.e., `let` bindings at the agent level) in this initial version, or are they purely stateless collections of behaviors? (This RFC assumes the latter for simplicity, with state being captured in closures or passed as arguments).
2.  What is the precise format of the metadata passed with the `AGENT_INVOKE` opcode? This will need to be defined in `tisc-spec.md`.
