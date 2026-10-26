#!/usr/bin/env python3
"""Reversible six-trit stream and 10-bit control with equal uint64 count framing.

Digit d=t+1 in {0,1,2}; least significant trit first. Consecutive
values share groups of five trits. Final unused digits must be zero.
"""
import json
import struct
from pathlib import Path


def encode(values):
    values = list(values)
    out = bytearray(struct.pack('<Q', len(values)))
    group, power = 0, 1
    for value in values:
        if type(value) is not int or not -364 <= value <= 364:
            raise ValueError('six-trit range required')
        code = value + 364  # sum(t_i+1)*3**i = value + (3**6-1)/2
        for _ in range(6):
            code, digit = divmod(code, 3)
            group += digit * power
            power *= 3
            if power == 243:
                out.append(group)
                group, power = 0, 1
    if power != 1:
        out.append(group)
    return bytes(out)


def decode(blob):
    if len(blob) < 8:
        raise ValueError('missing count')
    count, = struct.unpack('<Q', blob[:8])
    if len(blob) != 8 + (6*count+4)//5:
        raise ValueError('invalid length')
    if any(b >= 243 for b in blob[8:]):
        raise ValueError('invalid Base-243 byte')
    result, value, place = [], 0, 1
    pos = 0
    for byte in blob[8:]:
        for _ in range(5):
            byte, digit = divmod(byte, 3)
            if pos < 6*count:
                value += digit*place
                place *= 3
                if place == 729:
                    result.append(value-364)
                    value, place = 0, 1
            elif digit:
                raise ValueError('nonzero padding')
            pos += 1
    return result


def encode_binary10(values):
    values = list(values)
    out = bytearray(struct.pack('<Q',len(values)))
    acc, bits = 0, 0
    for v in values:
        if type(v) is not int or not -512 <= v <= 511:
            raise ValueError('10-bit range required')
        acc |= (v & 1023) << bits
        bits += 10
        while bits >= 8:
            out.append(acc & 255)
            acc >>= 8
            bits -= 8
    if bits:
        out.append(acc)
    return bytes(out)


def decode_binary10(blob):
    if len(blob) < 8: raise ValueError('missing count')
    n, = struct.unpack('<Q',blob[:8])
    if len(blob) != 8+(10*n+7)//8: raise ValueError('invalid length')
    acc, bits, values = 0, 0, []
    for b in blob[8:]:
        acc |= b << bits
        bits += 8
        while bits >= 10:
            v = acc & 1023
            values.append(v-1024 if v & 512 else v)
            acc >>= 10
            bits -= 10
    if acc: raise ValueError('nonzero padding')
    return values


def experiment():
    rows = []
    for n in [0,1,2,3,4,5,10,100,729,1000]:
        values = [(i % 729)-364 for i in range(n)]
        a, b = encode(values), encode_binary10(values)
        if decode(a) != values or decode_binary10(b) != values:
            raise RuntimeError('round-trip failed')
        rows.append(dict(values=n, base243_payload_bytes=len(a)-8,
                         binary10_payload_bytes=len(b)-8,
                         base243_framed_bytes=len(a), binary10_framed_bytes=len(b)))
    return dict(count_header_bytes=8, metadata_in_payload=False, rows=rows,
                scope='actual encoded lengths and round trips; no execution performance measurement')


if __name__ == '__main__':
    print(json.dumps(experiment(), indent=2))
