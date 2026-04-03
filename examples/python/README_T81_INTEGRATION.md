# T81 Integrated Bundle Marketplace

This directory contains the **T81-integrated bundle marketplace** that connects external AI models with the canonical T81 decision substrate.

## What This Does

Unlike the mock marketplace (`bundle_marketplace.py`), this version:

1. **Uses Real T81 Runtime** - Creates actual canonical bundles using `t81 ai task assess-fixed`
2. **Stores in CanonFS** - All artifacts are stored as content-addressed objects
3. **Follows Bundle Contract** - Implements the AI OS-Object Bundle Consumption Contract
4. **Provides Provenance** - Complete audit trail from input to decision

## Files

- `t81_integrated_bundle_marketplace.py` - Main marketplace implementation
- `example_model_config.json` - Sample external model configuration
- `example_input_data.json` - Sample access request data
- `demo_t81_integration.sh` - Complete demonstration script

## Quick Start

### Prerequisites

Build T81 with required targets:

```bash
cmake -S . -B build -G Ninja -DT81_BUILD_EXAMPLES=ON
cmake --build build --target t81 t81_make_assess_fixed_demo
```

### Run Demo

```bash
./examples/python/demo_t81_integration.sh
```

This will:
1. Create an assess-fixed bundle using external model integration
2. Store all artifacts in CanonFS
3. Retrieve and consume the bundle following the contract
4. Show the complete provenance chain

## Usage

### Create Assess-Fixed Bundle

```bash
python3 examples/python/t81_integrated_bundle_marketplace.py \
    create-assess-fixed \
    examples/python/example_model_config.json \
    examples/python/example_input_data.json
```

### Get Bundle Details

```bash
python3 examples/python/t81_integrated_bundle_marketplace.py \
    get-bundle <bundle_ref>
```

### Consume Bundle (Contract-Compliant)

```bash
python3 examples/python/t81_integrated_bundle_marketplace.py \
    consume-bundle <bundle_ref>
```

## Bundle Schema Support

Currently supports the **admitted bounded AI OS-object family**:

- `t81.ai.task.assess-fixed.bundle.v1` - Security assessment decisions
- `t81.ai.task.route-fixed.bundle.v1` - Path selection decisions  
- `t81.ai.task.classify-fixed.bundle.v1` - Rule selection decisions

## Bundle Consumption Contract

This implementation follows the [AI OS-Object Bundle Consumption Contract](../../docs/reference/AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md):

1. **Bundle First** - Start from canonical bundle object
2. **Dereference as Needed** - Follow `record_ref` and `action_ref` only when required
3. **Family-Specific Reading** - Interpret `action_ref` based on bundle schema
4. **CanonFS References** - All refs are content-addressed CanonFS objects

## Integration with External Models

The marketplace wraps external AI models (OpenAI, Anthropic, etc.) with T81 provenance:

1. **External Model Call** - Decision made by external model
2. **Canonical Recording** - Result stored as T81 artifact
3. **Policy Enforcement** - Axion policy applied before bundle creation
4. **Bundle Creation** - Complete decision chain with provenance

## Example Bundle Content

```json
{
  "bundle_ref": "sha3-256:abc123...",
  "schema": "t81.ai.task.assess-fixed.bundle.v1",
  "source_result_ref": "sha3-256:def456...",
  "source_provenance_ref": "sha3-256:ghi789...",
  "action_ref": "sha3-256:jkl012...",
  "record_ref": "sha3-256:mno345..."
}
```

Each reference points to a CanonFS artifact containing:
- **source_result_ref**: AI task result
- **source_provenance_ref**: Execution evidence and policy trace
- **action_ref**: Host action artifact
- **record_ref**: Typed downstream record

## Next Steps

To extend this marketplace:

1. **Add Route-Fixed Support** - Implement `create_route_fixed_bundle()`
2. **Add Classify-Fixed Support** - Implement `create_classify_fixed_bundle()`
3. **Bundle Listing** - Implement CanonFS bundle enumeration
4. **External Model Integration** - Connect to real AI APIs
5. **Policy Templates** - Add compliance framework templates

## Relationship to Core T81

This marketplace demonstrates how external systems can:
- **Consume T81 Decisions** - Use bundles as canonical decision objects
- **Maintain Provenance** - Complete audit trail preserved
- **Follow Governance** - All decisions policy-gated
- **Ensure Determinism** - Identical inputs produce identical bundles

It shows T81's role as a **decision substrate** rather than just an inference runtime.
