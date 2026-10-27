import importlib.abc
import runpy
import sys
class BlockRuntimes(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        if fullname.split('.')[0] in {'wasmtime', 'witness', 'arm_a', 'arm_b', 't81'}:
            raise ImportError('runtime/producer import forbidden')
sys.meta_path.insert(0, BlockRuntimes())
sys.argv = ['verify.py', *sys.argv[1:]]
runpy.run_path('verify.py', run_name='__main__')
