"""Independent arbitrary-precision PCG/corpus differential check (30,000 pairs)."""
from pathlib import Path
import subprocess
import sys

MASK = (1 << 128) - 1
MULT = 0x2360ED051FC65DA44385C511560B53A9
SEED = 0x5438315F434D50335F53454544303131

class PCG:
    def __init__(self):
        self.state = ((3 + SEED) * MULT + 3) & MASK

    def next(self):
        old = self.state
        self.state = (old * MULT + 3) & MASK
        x = ((old >> 64) ^ old) & ((1 << 64) - 1)
        r = old >> 122
        return ((x >> r) | (x << ((-r) & 63))) & ((1 << 64) - 1)

    def bounded(self, n):
        threshold = (1 << 64) % n
        while True:
            x = self.next()
            if x >= threshold:
                return x % n


def corpus(p):
    g = PCG()
    for _ in range(10000):
        u = (g.next() >> 11) * 2.0 ** -53
        if p == 1:
            k = g.bounded(1000) + 1
            v = -100 - k if u < .20 else -100 + g.bounded(201) if u < .30 else 100 + k
            yield (p, v, 0)
        else:
            a = -(1 << 63) + (1 << 30) + g.bounded((1 << 64) - (1 << 31))
            k = g.bounded(1 << 30) + 1
            lo, hi = (.475, .525) if p == 0 else (1 / 3, 2 / 3)
            b = a + k if u < lo else a if u < hi else a - k
            assert -(1 << 63) <= b < (1 << 63)
            yield (p, a, b)

if __name__ == '__main__':
    run = subprocess.run([sys.argv[1], '--dump-corpus'], text=True, capture_output=True, check=True)
    actual = [tuple(map(int, line.split(','))) for line in run.stdout.splitlines()]
    expected = [pair for p in range(3) for pair in corpus(p)]
    for p in range(3):
        golden=(Path(__file__).resolve().parent/f'fixtures/corpus_{p}_first8.csv').read_text().splitlines()[1:]
        golden=[tuple(map(int,line.split(','))) for line in golden]
        assert expected[p*10000:p*10000+8] == golden
        assert actual[p*10000:p*10000+8] == golden
    assert actual == expected
    print('INDEPENDENT_CORPUS_PASS pairs=30000 golden_pairs=24')
