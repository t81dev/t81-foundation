# Bundle v1 Consumption Contract

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Bundle v1 Consumption Contract](#bundle-v1-consumption-contract)
  - [Purpose](#purpose)
  - [Bundle Structure](#bundle-structure)
    - [Required Fields](#required-fields)
    - [Schema Values](#schema-values)
  - [Consumer Obligations](#consumer-obligations)
    - [1. Schema Validation](#1-schema-validation)
    - [2. Reference Resolution](#2-reference-resolution)
    - [3. Record Schema Validation](#3-record-schema-validation)
    - [4. Field Extraction](#4-field-extraction)
      - [Assess-Fixed Record](#assess-fixed-record)
      - [Route-Fixed Record](#route-fixed-record)
      - [Classify-Fixed Record](#classify-fixed-record)
  - [Error Handling](#error-handling)
    - [Required Error Conditions](#required-error-conditions)
    - [Error Format](#error-format)
  - [Failure Semantics](#failure-semantics)
    - [Bundle Consumption Failure](#bundle-consumption-failure)
    - [Reference Resolution Failure](#reference-resolution-failure)
  - [Compatibility Rules](#compatibility-rules)
    - [v1 Compatibility](#v1-compatibility)
    - [Implementation Requirements](#implementation-requirements)
    - [Minimal Consumer](#minimal-consumer)
    - [Testing Requirements](#testing-requirements)
  - [Non-Goals](#non-goals)
  - [Version Boundary](#version-boundary)

<!-- T81-TOC:END -->


This specification defines the exact contract for consuming T81 decision bundles, version 1.

## Purpose

This contract ensures that any independent implementer can consume a v1 bundle correctly, fail predictably, and know when they are out of specification.

## Bundle Structure

Every v1 bundle MUST contain exactly these fields:

```json
{
  "schema": "t81.ai.task.{family}.bundle.v1",
  "source_result_ref": "sha3-256:...",
  "source_provenance_ref": "sha3-256:...", 
  "action_ref": "sha3-256:...",
  "record_ref": "sha3-256:..."
}
```

### Required Fields

- `schema`: Bundle family schema identifier
- `source_result_ref`: CanonFS reference to raw AI task result
- `source_provenance_ref`: CanonFS reference to policy execution evidence
- `action_ref`: CanonFS reference to canonical downstream action
- `record_ref`: CanonFS reference to typed decision record

### Schema Values

MUST be one of:
- `t81.ai.task.assess-fixed.bundle.v1`
- `t81.ai.task.route-fixed.bundle.v1` 
- `t81.ai.task.classify-fixed.bundle.v1`

## Consumer Obligations

### 1. Schema Validation

MUST validate bundle schema against allowed values above.
MUST reject bundles with unknown schemas.

### 2. Reference Resolution

MUST resolve all four `*_ref` fields through CanonFS.
MUST fail clearly if any reference does not resolve.

### 3. Record Schema Validation

MUST retrieve and validate the record schema:
- assess-fixed: `t81.ai.task.assess-fixed.host-action-record.v1`
- route-fixed: `t81.ai.task.route-fixed.path-selection-record.v1`
- classify-fixed: `t81.ai.task.classify-fixed.rule-selection-record.v1`

MUST reject records with unexpected schemas.

### 4. Field Extraction

MUST extract only defined fields from records:

#### Assess-Fixed Record
- `selected_action` (required): string
- `selected_path` (required): string
- `decision` (required): "ALLOW"|"DENY"|"REVIEW"
- `reason_code` (required): string
- `termination_reason` (required): string

#### Route-Fixed Record  
- `selected_action` (required): string
- `selected_path` (required): string
- `route` (required): string
- `termination_reason` (required): string

#### Classify-Fixed Record
- `selected_rule_set` (required): string
- `rule_set_ref` (required): string
- `label` (required): string
- `termination_reason` (required): string

## Error Handling

### Required Error Conditions

MUST fail with these specific error codes:

| Condition | Error Code | Message |
|------------|-------------|---------|
| Bundle schema not found | BUNDLE_UNKNOWN_SCHEMA | Bundle schema {schema} not supported |
| Missing required field | BUNDLE_MISSING_FIELD | Bundle missing required field: {field} |
| Invalid reference format | BUNDLE_INVALID_REF | Bundle reference {ref} not valid CanonFS format |
| Reference resolution failure | BUNDLE_REF_NOT_FOUND | Bundle reference {ref} not found in CanonFS |
| Record schema mismatch | RECORD_SCHEMA_MISMATCH | Record schema {actual} != expected {expected} |
| Missing record field | RECORD_MISSING_FIELD | Record missing required field: {field} |
| Invalid field value | RECORD_INVALID_VALUE | Field {field} has invalid value: {value} |

### Error Format

Errors MUST be structured:
```
ERROR_CODE: Human readable message
```

## Failure Semantics

### Bundle Consumption Failure

If ANY step fails, consumer MUST:
1. NOT consume any partial data
2. Report specific error code and message
3. NOT attempt to guess or infer missing data
4. Preserve original bundle reference for debugging

### Reference Resolution Failure

If CanonFS reference fails:
1. Report BUNDLE_REF_NOT_FOUND with specific ref
2. Include CanonFS root path in error context
3. NOT attempt fallback or alternative resolution

## Compatibility Rules

### v1 Compatibility

Consumers MUST accept any bundle that:
- Has valid v1 schema
- Contains all required fields
- Has resolvable CanonFS references
- Has valid record schema and fields

Consumers MUST NOT:
- Require additional fields beyond specification
- Accept malformed references
- Infer missing data
- Modify bundle content during consumption

### Implementation Requirements

### Minimal Consumer

Any v1 consumer MUST implement:
1. Bundle schema validation
2. CanonFS reference resolution
3. Record schema validation  
4. Required field extraction
5. Structured error reporting

### Testing Requirements

Consumers MUST be tested against:
- Valid minimal bundles (all families)
- Bundles with missing required fields
- Bundles with invalid references
- Bundles with wrong record schemas
- Bundles with malformed field values

## Non-Goals

This contract does NOT cover:
- Bundle creation or storage
- Performance optimization
- Transport protocols
- Security beyond reference validation
- Future schema versions

## Version Boundary

Changes requiring new version:
- Adding/removing bundle fields
- Changing bundle schema format
- Modifying required field semantics
- Changing reference resolution semantics

Changes allowed in v1:
- New record field types (if backward compatible)
- Additional validation rules
- Better error messages
- Performance improvements
