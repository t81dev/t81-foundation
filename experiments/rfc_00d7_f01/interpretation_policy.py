"""Strict metadata handling; no candidate, reference or domain imports."""

SUPPORTED_KEYS = (
    'O1-race', 'event-kind-payload', 'nested-LIP-coordinate', 'outside-arena',
    'slot-lifecycle', 'acknowledgement', 'digest-text', 'partial-write-continuation',
)


def validate(values, declarations):
    if type(values) is not dict or set(values) != set(SUPPORTED_KEYS):
        raise ValueError('BLOCKED: unknown or missing interpretation key')
    if set(declarations) != set(SUPPORTED_KEYS):
        raise ValueError('BLOCKED: interpretation registry mismatch')
    for key in SUPPORTED_KEYS:
        value = values[key]
        alternatives = declarations[key]['alternatives']
        if value is not None and value not in alternatives:
            raise ValueError('BLOCKED: unknown interpretation value: ' + key)
        if key != 'outside-arena' and value is not None:
            raise ValueError('BLOCKED: selected interpretation is not implemented: ' + key)


def outside_arena(values):
    # Exact declared key. No .get(), alias, default or normalization.
    return values['outside-arena']
