"""Instrumentation only; no candidate or reference semantics."""

import json
import os
import time

STATE_CAPS = dict(zip('ABCDEF', (4096, 4096, 8192, 16384, 16384, 256)))
WALL_CAPS = dict(zip('ABCDEF', (5, 5, 10, 10, 10, 10)))
MIB = 1024 * 1024


class Limit(Exception):
    pass


class Budget:
    def __init__(self):
        self.started = time.monotonic_ns()
        self.cpu_started = time.process_time_ns()
        self.states = {area: 0 for area in STATE_CAPS}
        self.bytes = {area: 0 for area in STATE_CAPS}
        self.metadata_bytes = 0
        self.area = None

    def enter(self, area):
        self.area = area
        self.area_started = time.monotonic_ns()

    def checkpoint(self):
        now = time.monotonic_ns()
        if now - self.started >= 60_000_000_000:
            raise Limit('suite wall deadline')
        if now - self.area_started >= WALL_CAPS[self.area] * 1_000_000_000:
            raise Limit('area wall deadline')
        # Supervisor meters aggregate cgroup CPU independently. Worker soft
        # threshold is conservative; no 50s physical precision claim is made.
        if time.process_time_ns() - self.cpu_started >= 44_000_000_000:
            raise Limit('worker conservative CPU stop')

    def visit(self, count=1):
        self.checkpoint()
        if count < 1 or self.states[self.area] + count > STATE_CAPS[self.area]:
            raise Limit('state cap before materialization')
        self.states[self.area] += count
        if sum(self.states.values()) > 49408:
            raise Limit('suite state cap')

    def reserve(self, size, area=None):
        if size < 0:
            raise ValueError('negative write')
        if area is not None and self.bytes[area] + size > 4 * MIB:
            raise Limit('area artifact cap')
        total = sum(self.bytes.values()) + self.metadata_bytes
        # Reserve 64 KiB for suite terminal metadata when writing area data.
        if total + size > 25 * MIB - 65536:
            raise Limit('suite artifact cap')
        if area is None:
            self.metadata_bytes += size
        else:
            self.bytes[area] += size


class Evidence:
    def __init__(self, directory, budget):
        self.root = directory
        self.budget = budget
        self.witness_bytes = {area: 0 for area in STATE_CAPS}
        self.ledger = {}
        self.hashes = {}

    def write(self, name, value, area=None, witness=False):
        raw = (json.dumps(value, sort_keys=True, separators=(',', ':'),
                          ensure_ascii=True) + '\n').encode('ascii')
        if witness and self.witness_bytes[area] + len(raw) > 512 * 1024:
            raise Limit('raw witness cap')
        self.budget.reserve(len(raw), area)
        # New private scratch directory is created by supervisor, no overwrite.
        fd = os.open(self.root / name, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
        with os.fdopen(fd, 'wb') as handle:
            handle.write(raw)
        if witness:
            self.witness_bytes[area] += len(raw)

    def case(self, area, number, digest, result, category, subclaim):
        raw = f'{number:08d} {digest} {category} {result} {subclaim}\n'.encode('ascii')
        if len(raw) > 128:
            raise Limit('ledger record exceeds 128 bytes')
        self.budget.reserve(len(raw), area)
        if area not in self.ledger:
            fd = os.open(self.root / f'{area}.ledger',
                         os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
            self.ledger[area] = os.fdopen(fd, 'wb')
        self.ledger[area].write(raw)
        self.ledger[area].flush()

    def close(self):
        for handle in self.ledger.values():
            handle.close()
