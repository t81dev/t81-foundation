# RFC-00D0: Base-81-Aware TCP/IP Stack

**Status:** draft
**Type:** standards-track
**Applies-To:** TernaryOS network subsystem, driver/runtime boundary, user-facing network identity and tooling
**Created:** 2026-03-26
**Updated:** 2026-03-26
**Author:** @t81dev
**Discussion:** initial scope draft

---

## 1. Summary

This RFC proposes a standards-compatible TCP/IP stack for TernaryOS with a
T81-native identity, representation, and observability layer.

The stack remains conventional on the wire:

- Ethernet, ARP, IPv4, ICMP, UDP, and TCP retain standard binary semantics
- packet layout, checksums, ports, sequence numbers, and transport state
  machines are not redefined around base-81 concepts

The T81-specific layer appears above and around the wire protocol surface:

- base-81 host, service, interface, and session identifiers
- base-81 canonical address rendering for tooling and logs
- base-81-aware service discovery and endpoint descriptors
- canonical event, packet, and flow evidence suitable for replay and audit

This RFC does not yet freeze a syscall ABI, socket API, or packet-capture file
format. It establishes the architectural direction and the boundaries that
future implementation work must preserve.

Concrete base-81 endpoint and evidence encodings are included as illustrative
examples in Appendix A. Those examples are not yet normative, but they show the
 intended direction for user-facing notation and trace artifacts.

## 2. Motivation

TernaryOS needs a networking story that is both practical and native to the
rest of the system.

Purely conventional TCP/IP integration would provide interoperability, but it
would also leave networking as an outlier subsystem with foreign naming,
foreign diagnostics, and weak integration with T81 identity and audit
primitives.

A fully custom "base-81 network stack" would move too far in the other
direction. It would sacrifice interoperability and blur the contract being
implemented. If the system claims to provide TCP/IP, then TCP/IP wire semantics
must remain recognizable and standards-compatible.

The middle path is therefore:

- conventional TCP/IP on the wire
- T81-native semantics for naming, description, tracing, and control surfaces
- deterministic internal behavior when driven by a recorded event/timer stream

That split preserves interoperability while making networking feel like part of
the T81 system rather than a bolted-on compatibility module.

## 3. Goals

- Provide a standards-compatible TCP/IP stack suitable for TernaryOS runtime
  and service communication
- Introduce base-81 as a first-class identity and representation layer around
  the stack
- Define a replayable internal event model for debugging, audit, and
  conformance work
- Keep wire compatibility separate from T81-native service and tooling
  semantics
- Leave room for a future higher-level T81 overlay protocol family above TCP/IP

## 4. Non-Goals

- Redefining Ethernet, ARP, IP, UDP, or TCP wire fields in base-81 terms
- Claiming deterministic network outcomes across real external networks
- Freezing a final userland sockets API in this initial draft
- Standardizing IPv6 in v1 of this RFC
- Defining a T81-native application-layer protocol suite in this document

## 5. Architectural Position

### 5.1 Wire Compatibility

The network stack defined by this RFC is a TCP/IP implementation, not a
translation layer for a distinct T81 wire protocol.

That means:

- Ethernet frames remain Ethernet frames
- ARP remains ARP
- IPv4 packet encoding remains standard
- UDP and TCP transport semantics remain standard

Any T81-native identity or naming layer must resolve onto standard protocol,
address, and port tuples before transmission.

### 5.2 Base-81 Placement

Base-81 is permitted and encouraged in the following roles:

1. identity
   - host identifiers
   - service identifiers
   - interface labels
   - flow/session identifiers

2. representation
   - canonical textual rendering of addresses and endpoints
   - compact shell-friendly endpoint notation
   - stable packet/flow reference strings

3. discovery and policy
   - service registry entries
   - endpoint descriptor manifests
   - route, capability, or trust labels

4. observability
   - packet capture identifiers
   - flow trace identifiers
   - replay logs
   - evidence exports

Base-81 is not part of the normative binary wire format for TCP/IP itself.

### 5.3 Determinism Position

This RFC distinguishes between:

- deterministic stack execution
- deterministic network behavior

The stack may be required to behave deterministically when driven by the same
recorded sequence of:

- received packets
- timer expirations
- local send requests
- device up/down events

This RFC does not require the external network to produce deterministic inputs,
and it does not promise replay-stable outcomes against arbitrary peers.

The intended property is:

- same event/timer trace in
- same internal state transitions, packet emissions, and audit artifacts out

## 6. Proposed Layers

### 6.1 Link and Internet Layer

The initial protocol scope for the stack is:

- Ethernet
- ARP
- IPv4
- ICMPv4

These layers are responsible for:

- frame parsing and validation
- neighbor resolution
- packet routing/dispatch
- MTU-aware packet handling
- error and reachability signals required by upper layers

### 6.2 Transport Layer

The initial transport scope is:

- UDP
- TCP

UDP should provide the simplest end-to-end standards-compatible datagram path.
TCP should provide a stream transport with standard connection setup, ordered
delivery, retransmission, and teardown semantics.

### 6.3 T81 Identity and Resolution Layer

Above the raw endpoint layer, TernaryOS should provide a T81-native resolution
surface that maps:

- base-81 host or node identities
- base-81 service identifiers
- optional policy and trust metadata

onto:

- protocol kind
- address
- port
- optional route/interface preference

This allows T81 tools to operate on native names while preserving standard
network interoperability underneath.

The current architectural bias is toward immutable CanonFS-backed service
descriptors and resolution manifests rather than a mutable runtime-first
registry.

That bias is motivated by:

- content-addressed identity
- auditable configuration history
- deterministic lookup inputs for replay and conformance
- consistency with existing CanonFS-centered artifact governance

A runtime registry is not forbidden, but if introduced later it should be
treated as an explicitly governed dynamic layer above the canonical manifest
surface rather than the primary source of truth.

### 6.4 CanonFS Service Descriptor Contract

The preferred v1 resolution input is a CanonFS-backed immutable service
descriptor manifest.

This RFC defines a first concrete descriptor contract named
`t81.net.service-descriptor.v1`.

The initial authoring and interchange format should be UTF-8 JSON with an
explicit top-level `schema` field, consistent with other versioned tool-facing
surfaces in the repository.

Each descriptor should identify:

- a schema id and version
- a base-81 service identity
- a base-81 host or node identity
- a transport protocol
- a resolved network coordinate
- optional policy, trust, and routing hints
- the CanonFS object identity of the descriptor itself

Required logical fields:

```text
schema: t81.net.service-descriptor.v1
service_id: <base81-service-id>
host_id: <base81-host-id>
transport: tcp | udp
address_family: ipv4
address: <standard textual IP literal>
port: <uint16>
manifest_ref: <canonfs object reference>
```

Optional logical fields:

```text
interface_id: <optional base81-interface-id>
route_label: <optional base81-route-label>
policy_profile: <optional policy/profile ref>
trust_profile: <optional trust/profile ref>
labels: <optional array of base81 labels>
```

Descriptor rules:

1. `service_id` and `host_id` are the primary T81-facing identities.
2. `transport`, `address_family`, `address`, and `port` are the standard
   networking coordinates used for actual packet transmission.
3. `manifest_ref` is the stable CanonFS identity of the descriptor artifact
   used during resolution and should appear in audit and replay records.
4. Unknown required fields must cause rejection. Unknown optional fields may be
   ignored only if the schema version permits them.
5. Multiple descriptors may exist for one `service_id`, but selection rules
   must be deterministic for a fixed manifest set and policy state.

The likely design direction is that a resolver consumes one or more CanonFS
descriptors, applies deterministic filtering and policy, and produces a single
resolved endpoint tuple plus the `manifest_ref` lineage required for evidence.

#### 6.4.1 JSON Shape

Illustrative `t81.net.service-descriptor.v1` JSON document:

```json
{
  "schema": "t81.net.service-descriptor.v1",
  "service_id": "B6nP4",
  "host_id": "Q2xA8",
  "transport": "tcp",
  "address_family": "ipv4",
  "address": "192.0.2.14",
  "port": 25,
  "interface_id": "if:7Lm",
  "route_label": "rt:primary",
  "policy_profile": "default-egress",
  "trust_profile": "tier1-peer",
  "manifest_ref": "canonfs:K81A2M4...",
  "labels": ["mail", "internal"]
}
```

The serialization format may evolve later, but any alternative encoding should
preserve the same field semantics and schema versioning boundary.

#### 6.4.2 Field Semantics

- `schema`
  - required string
  - must equal `t81.net.service-descriptor.v1`
- `service_id`
  - required string
  - must be a valid base-81 service identifier
  - identifies the T81-facing service name used by tools and resolution
- `host_id`
  - required string
  - must be a valid base-81 host or node identifier
  - identifies the target host identity within the T81 naming layer
- `transport`
  - required string
  - allowed values: `tcp`, `udp`
- `address_family`
  - required string
  - initial allowed value: `ipv4`
- `address`
  - required string
  - must be a syntactically valid address literal for the declared family
- `port`
  - required integer
  - must be in the inclusive range `1..65535`
- `manifest_ref`
  - required string
  - must be a valid CanonFS object reference
  - should identify the descriptor artifact used for resolution and evidence
- `interface_id`
  - optional string
  - if present, must be a valid base-81 interface identifier
- `route_label`
  - optional string
  - if present, must be a valid base-81 route label
- `policy_profile`
  - optional string
  - if present, names the policy profile that should constrain use of this
    descriptor
- `trust_profile`
  - optional string
  - if present, names the trust classification for the endpoint
- `labels`
  - optional array of strings
  - each entry should be a stable operator-facing alias or classification label

#### 6.4.3 Validation Rules

A descriptor must be rejected if:

- `schema` is missing or unknown
- any required field is missing
- `transport` is not one of the declared allowed values
- `address_family` is not supported by the implementation
- `address` is invalid for the declared family
- `port` is outside the allowed range
- `manifest_ref` is not a valid CanonFS reference
- any required identity field fails base-81 validation

A descriptor may be rejected if:

- `policy_profile` or `trust_profile` names an unknown local policy surface
- `interface_id` names an interface that does not exist in the current runtime
- the implementation detects conflicting duplicate descriptors with no valid
  deterministic tie-break path

Rejecting a malformed or unsupported descriptor is preferred over silently
coercing it into a weaker interpretation.

### 6.5 Resolution Semantics

Given a fixed set of CanonFS service descriptors and a fixed policy state,
resolution should be deterministic.

For a request such as:

- host identity `Q2xA8`
- service identity `B6nP4`
- transport preference `tcp`

the resolver should:

1. load the authoritative descriptor set
2. filter by matching identity fields
3. reject entries that violate policy or capability constraints
4. apply a deterministic tie-break rule if multiple candidates remain
5. return the selected endpoint plus the manifest references used

The tie-break rules are not yet frozen, but the RFC direction is:

- no host-order-dependent iteration
- no wall-clock-dependent selection
- no "first registry response wins" behavior

This matters because connection setup evidence must be explainable after the
fact. If a service identity resolves to one address in one replay and another
in the next, the system loses one of the main benefits of a governed network
subsystem.

The current v1 preference is:

1. exact match on `service_id`
2. exact match on requested `host_id` if one was supplied
3. exact match on requested `transport` if one was supplied
4. policy and capability filtering
5. deterministic ordering by:
   - `host_id`
   - `service_id`
   - `transport`
   - `address_family`
   - `address`
   - `port`
   - `manifest_ref`
6. select the first surviving candidate in that canonical order

That ordering is intentionally simple. It can be revised later, but any future
revision must remain deterministic and auditable.

### 6.6 Observability and Replay Layer

The stack should emit canonical network evidence records for:

- link events
- packet ingress and egress
- transport state transitions
- timer scheduling and expiry
- connection lifecycle changes
- policy or capability denials

These records should be sufficient to support:

- deterministic replay of stack behavior under recorded inputs
- post-mortem audit of connection failures
- conformance and interoperability harnesses

The preferred direction is to integrate these records with existing Axion
evidence conventions rather than invent a parallel observability system for
networking.

At minimum, the replay/evidence model should be able to capture:

- ingress packet bytes and ingress interface identity
- egress intent and emitted packet bytes
- timer creation, cancellation, and expiration
- transport state transitions and connection-local identifiers
- policy decisions that affect delivery, bind, connect, listen, or transmit
- the CanonFS identity of the service-resolution manifest used for the decision

The design target is not just readable logs. The design target is a canonical
event stream rich enough that the same recorded network inputs can reproduce the
same stack-level behavior and evidence output on replay.

## 7. Base-81 Surfaces

### 7.1 High-Value v1 Surfaces

The following surfaces are explicitly in scope for base-81 enhancement:

- host and service naming
- canonical address and endpoint rendering
- shell and CLI network commands
- trace and packet evidence identifiers
- service discovery descriptors
- route and interface labels

### 7.2 Deferred or Avoided Surfaces

The following areas should not be base-81-specialized in v1:

- raw packet header fields
- TCP sequence number mechanics
- checksum algorithms
- congestion-control arithmetic
- retransmission and RTT internals

Those are transport and protocol mechanics, not representation or identity
surfaces.

## 8. Open Design Questions

This initial draft intentionally leaves several decisions open:

1. precise base-81 rendering grammar
   - exact alphabet and reserved characters
   - shell-safe endpoint rendering rules
   - how service aliases and raw port renderings coexist

2. replay format and evidence schema
   - text-first event stream
   - binary capture with canonical textual projection
   - integration with existing Axion and CanonFS evidence models

3. concrete syscall/library surface
   - exact syscall ABI shape
   - exact CLI grammar
   - exact library/API signatures

These questions should be resolved before the RFC advances beyond `draft`.

## 9. User-Facing API Direction

This RFC adopts a layered API direction rather than choosing exclusively
between a BSD-socket-like surface and a T81-native networking surface.

The intended architecture is:

1. a conventional transport-facing API
2. a T81-native identity and resolution API layered above it

This gives TernaryOS:

- a recognizable compatibility boundary for standard networking concepts
- a native path for base-81 service naming and governed resolution
- a clean split between transport mechanics and T81 identity semantics

### 9.1 Conventional Transport Surface

The lower user-facing boundary should expose conventional networking concepts
such as:

- bind
- listen
- accept
- connect
- send
- receive
- close

This does not require a byte-for-byte POSIX socket API clone in the first
implementation, but it does require that the transport-facing boundary remain
clearly legible in standard networking terms.

The transport surface should operate on resolved endpoint coordinates such as:

- transport kind
- address family
- address
- port

This surface exists for:

- interoperability
- familiar implementation/testing patterns
- clear mapping onto TCP and UDP semantics

### 9.2 T81-Native Resolution and Session Surface

Above the transport surface, TernaryOS should provide a native API that works
in terms of:

- `service_id`
- `host_id`
- base-81 endpoint references
- optional policy/trust hints
- session and flow references

This native layer should:

- resolve T81 identities through CanonFS-backed service descriptors
- attach policy and evidence context to the resolved session
- return stable session/flow identifiers suitable for shell and audit surfaces

Illustrative operations in this layer:

- `net resolve B6nP4`
- `net connect B6nP4`
- `net connect Q2xA8:B6nP4`
- `net session show flow:9Qm2Lx`

These examples are non-normative, but they show the intended direction:
operators and T81-native tools should not need to work directly with raw
`ip:port` tuples unless they explicitly choose to.

### 9.3 Layering Contract

The T81-native networking surface should lower into the conventional
transport-facing surface rather than bypassing it with a separate transport
implementation model.

That means:

- service resolution produces a conventional endpoint tuple
- the transport layer performs the actual bind/connect/send/receive work
- audit and evidence records should retain both:
  - the original T81-facing identity inputs
  - the resolved transport coordinates

This is important because it preserves both explainability and compatibility.
A session should be auditable as:

- "user requested service `B6nP4` on host `Q2xA8`"
- "resolver selected `tcp`, `192.0.2.14`, port `25`, manifest `canonfs:...`"

not merely as a raw transport event divorced from the T81 naming layer.

### 9.4 API Stability Boundary

This RFC does not freeze concrete syscall numbers, CLI syntax, or library
function signatures for the networking API.

It does freeze the architectural expectation that:

- T81-native identity and service resolution are first-class
- conventional transport semantics remain directly representable
- the native layer is additive and lowering-based, not a replacement for TCP/IP
  transport concepts

### 9.5 Error Model Direction

The layered API should preserve enough detail that failures can be reported in
both T81-native and transport-native terms.

Examples:

- resolution failure: no matching `service_id`
- policy failure: descriptor exists but use is denied by current policy
- transport failure: TCP connect timeout to resolved endpoint
- runtime failure: interface unavailable for requested route/interface binding

This matters because a governed networking surface should let operators answer
two different questions:

- "why did the T81 service request fail?"
- "what happened at the transport layer after resolution?"

The reporting model should make a hard distinction between:

- pre-transport denial
  - resolution or policy prevented a transport attempt from occurring
- transport-layer failure
  - a concrete endpoint was selected and the transport operation failed after
    that point

Illustrative distinction:

- policy denial
  - request: `net connect Q2xA8:B6nP4`
  - resolution: descriptor found
  - policy result: denied by `trust_profile=tier1-peer`
  - transport attempt: none
- transport timeout
  - request: `net connect Q2xA8:B6nP4`
  - resolution: `tcp 192.0.2.14:25` via `canonfs:K81A2M4...`
  - policy result: allowed
  - transport result: timeout during TCP connect

This distinction should also appear in evidence records so that audits can
differentiate:

- "the system refused to attempt the connection"
- "the system attempted the connection and the network/path failed"

## 10. Deployment Strategy

This RFC adopts a staged deployment strategy.

The preferred initial implementation target is virtual or internal networking
first, followed by external NIC-backed networking later.

### 10.1 Phase 1: Virtual/Internal Networking

The first implementation should prioritize network paths where both endpoints
and most event sources are inside the TernaryOS-controlled environment.

Examples include:

- service-to-service communication within TernaryOS
- loopback and local virtual interfaces
- test harnesses and simulated peers
- controlled packet/timer replay environments

This phase is preferred because it allows the project to validate:

- base-81 identity and service resolution
- CanonFS-backed descriptor loading
- session/flow evidence capture
- deterministic replay behavior under fixed traces
- API ergonomics for both transport-facing and T81-native layers

without immediately taking on the full nondeterminism and device complexity of
external networks.

### 10.2 Phase 2: External NIC-Backed Networking

External networking should follow after the internal model is stable.

This phase extends the same architectural contract to:

- real link devices
- external peers
- real packet timing and loss conditions
- device-driver-mediated ingress and egress

The purpose of delaying this phase is not to avoid external networking. The
purpose is to avoid coupling the first architectural validation of the stack to
all external hardware and network variability at once.

### 10.3 Architectural Requirement Across Phases

The internal-first deployment strategy must not create a throwaway architecture.

Phase 1 and Phase 2 should share:

- the same service descriptor contract
- the same resolution semantics
- the same layering boundary
- the same evidence/replay model
- the same transport-state machine expectations

The difference between phases should primarily be the device and peer boundary,
not the meaning of service identity or connection lifecycle.

### 10.4 Rationale

An internal-first strategy is the most defensible path for this project
specifically because TernaryOS places unusual weight on:

- determinism-aware execution
- traceability
- policy enforcement
- CanonFS-backed identity and provenance

Those properties are easier to establish in a controlled virtual environment
before extending them across uncontrolled external networks.

### 10.5 Risks

The main risk of an internal-first strategy is overfitting to simulated or
well-behaved environments.

To limit that risk, the project should ensure that Phase 1 tests include:

- reorder and delay injection
- packet loss and retransmission scenarios
- malformed packet handling
- descriptor and policy failure paths
- mixed success/failure connection sequences

That keeps the initial stack honest without forcing full hardware bring-up on
day one.

## 11. Impact

### 11.1 Compatibility

This RFC is designed to maximize interoperability by preserving conventional
TCP/IP wire semantics.

The T81-specific layer is additive:

- better naming
- better tooling
- better auditability

not a replacement for standard networking behavior.

### 11.2 Implementation Cost

This direction adds complexity relative to a minimal host-socket wrapper because
it requires:

- an internal packet and timer model
- evidence emission
- service-resolution machinery
- T81-native naming and rendering surfaces

That cost is justified only if TernaryOS intends networking to be a native
subsystem rather than a thin compatibility shim.

### 11.3 Governance and Testing

This RFC will eventually require:

- conformance tests for protocol behavior
- replay tests for internal determinism under fixed traces
- interoperability tests against standard peers
- governance boundaries defining which network artifacts are part of the
  deterministic surface and which are not
- phased evidence showing that the internal-first deployment path does not
  diverge semantically from later external-network execution

## 12. Alternatives Considered

### 12.1 Pure Host-Socket Delegation

Rejected as the sole architecture.

It provides quick utility but gives TernaryOS little control over evidence,
traceability, or T81-native identity. It is acceptable as a bootstrap aid, not
as the architectural endpoint.

### 12.2 Fully Custom Base-81 Wire Protocol

Rejected for the TCP/IP RFC.

That would be a different protocol family, not TCP/IP. If pursued, it should be
specified later as an overlay or adjacent transport above standard networking.

### 12.3 Full Network Determinism as a Contract

Rejected.

External networks are inherently nondeterministic. The correct contract is
deterministic internal stack execution under a recorded event/timer trace, not a
promise that real-world peer interactions always replay identically.

## 13. Next Steps

Before this RFC should move from `draft` to `proposed`, follow-on work should
settle:

- the precise base-81 rendering grammar for endpoints, aliases, and flow/session
  references
- the replay/evidence schema format and its integration with Axion/governance
  surfaces
- the exact syscall, CLI, and library/API signatures that realize the layered
  model

## Appendix A. Illustrative Base-81 Surface Examples

This appendix is non-normative. It exists to make the intended user-facing
benefits concrete before the exact encoding is frozen.

### A.1 Endpoint Rendering

Conventional endpoint:

```text
192.0.2.1:443/tcp
```

Illustrative T81-facing rendering:

```text
tcp://3Kp8N.7f:tls
```

Where:

- `3Kp8N` is an illustrative base-81 host identifier
- `7f` is an illustrative compact rendered address fragment
- `tls` is a service alias that resolves to port `443`

The exact split between rendered address material and service alias remains open
for later drafts.

The likely rendering goals are:

- shell-safe by default
- compact enough for CLI output and logs
- unambiguous between service-alias and raw-port forms

The precise base-81 alphabet and rendering grammar remain open. This RFC only
commits to the need for a canonical rendering, not yet the final character set.

#### A.1.1 Rendering Goals

The rendering grammar should optimize for:

- shell safety
  - avoid characters that commonly require quoting or escaping in interactive
    shells
- compactness
  - keep the common rendered form short enough for logs and CLI status output
- readability
  - preserve enough visual structure that operators can distinguish host,
    service, and transport components quickly
- explicit fallback
  - support both service-alias rendering and a raw-port rendering when no alias
    exists or when an operator requests a transport-literal form

Illustrative sketch:

```text
endpoint     := transport "://" host "." service
endpoint_raw := transport "://" host ":" port
flow_ref     := "flow:" flow_id
packet_ref   := "pkt:" packet_id
```

This sketch is non-normative. It exists to clarify the shape of the intended
surface, not to freeze the exact grammar.

### A.2 Service Descriptor

Illustrative CanonFS-backed resolution descriptor:

```text
schema = "t81.net.service-descriptor.v1"
service_id = "B6nP4"
host_id = "Q2xA8"
transport = "tcp"
address_family = "ipv4"
ipv4 = "192.0.2.14"
port = 25
interface_id = "if:7Lm"
route_label = "rt:primary"
policy_profile = "default-egress"
trust_profile = "tier1-peer"
manifest_ref = "canonfs:K81A2M4..."
```

The key idea is that operators interact with the base-81 identity surfaces,
while the resolver deterministically maps those identities to standard transport
coordinates.

### A.3 Flow and Trace Identifiers

Illustrative flow reference:

```text
flow:9Qm2Lx
```

Illustrative packet evidence reference:

```text
pkt:5fA1zK
```

Illustrative replay bundle reference:

```text
replay:net:2Mx81T
```

These identifiers are intended to be:

- compact enough for shell output
- stable enough for audit references
- canonical enough to survive export/import and replay

### A.3.1 Rendering Edge Cases

Illustrative raw-port fallback:

```text
tcp://Q2xA8:25
```

Illustrative service-alias form:

```text
tcp://Q2xA8.mail
```

Illustrative longer host/service identities:

```text
udp://A8kLm2Z.sync
```

The intended rule is that service-alias rendering should be preferred when the
alias is unambiguous in the current resolution context, while raw-port fallback
must remain available for debugging, low-level tooling, and unresolved alias
cases.

### A.4 Illustrative Resolve -> Connect -> Inspect Flow

Illustrative operator flow:

```text
$ net resolve Q2xA8:B6nP4
resolved service=B6nP4 host=Q2xA8 transport=tcp address=192.0.2.14 port=25 manifest=canonfs:K81A2M4...

$ net connect Q2xA8:B6nP4
session opened flow:9Qm2Lx endpoint=tcp://Q2xA8.mail

$ net session show flow:9Qm2Lx
flow:9Qm2Lx
  request: Q2xA8:B6nP4
  resolved: tcp 192.0.2.14:25
  manifest: canonfs:K81A2M4...
  policy: allowed
  state: established
```

Illustrative policy-denied flow:

```text
$ net connect Q2xA8:B6nP4
connect denied service=B6nP4 host=Q2xA8 policy=trust_profile:tier1-peer reason=peer-not-allowed
```

Illustrative transport-failure flow:

```text
$ net connect Q2xA8:B6nP4
connect failed service=B6nP4 host=Q2xA8 resolved=tcp 192.0.2.14:25 error=tcp-timeout
```

These examples are still non-normative. They exist to show the intended user
experience:

- T81-facing identity stays visible
- resolved transport coordinates are still inspectable
- policy denials and transport failures are visibly different classes of event

## 14. References

- RFC-0000: T81 Base-81 Ternary Computing Stack
- RFC-0002: Deterministic Execution Contract
- RFC-00B2: Device Drivers
- RFC-00B3: Axion Kernel Architecture
- RFC-00B4: Userland Service Contract
- RFC-0054: CanonFS Object Identity and Persistence Contract
