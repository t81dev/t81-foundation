"""CSV integration regressions; all values are synthetic, never measurements."""
import pathlib
import subprocess
import sys
import tempfile

HEADER = 'pattern,mode,repetition,instructions,branch_misses,elapsed_ns,checksum\n'

def rows():
    return [f'{p},{m},{r},{70 if m == 0 else 100},10,{50 if m == 0 else 55},42\n'
            for p in range(3) for m in range(5) for r in range(10)]


def main():
    cases = 0
    with tempfile.TemporaryDirectory() as directory:
        path = pathlib.Path(directory) / 'synthetic.csv'
        def check(lines, expected, code):
            nonlocal cases
            path.write_text(HEADER + ''.join(lines))
            result = subprocess.run([sys.argv[1], str(path)], capture_output=True, text=True)
            assert result.returncode == code, result.stdout + result.stderr
            assert expected in result.stdout, result.stdout
            cases += 1
        check(rows(), 'overall=SUPPORTED', 0)
        check(rows()[:-1], 'BLOCKED', 6)
        check(rows() + rows()[:1], 'BLOCKED', 6)
        check([], 'BLOCKED', 6)
        data = rows()
        data[0] = '0,0,0,nan,10,50,42\n'
        check(data, 'BLOCKED', 6)
        data = rows()
        data[-1] = data[-1].replace(',42\n', ',43\n')
        check(data, 'PAYLOAD_PARITY', 6)
        data = rows()
        for r in range(10):
            data[r] = f'0,0,{r},70,11,50,42\n'
        check(data, 'reason=MISS_REGRESSION', 0)
        # Fastest baseline is selected from all four, independently per pattern.
        data = rows()
        for r in range(10):
            data[40 + r] = f'0,4,{r},100,10,49,42\n'
        check(data, 'reason=LATENCY_REGRESSION', 0)
    print(f'ANALYSIS_INTEGRATION_PASS cases={cases}')

if __name__ == '__main__':
    main()
