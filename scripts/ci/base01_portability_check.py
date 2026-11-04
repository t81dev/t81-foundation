"""Correctness-only native platform execution; no benchmark targets or timers."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys

CORE = ['t81_sha3_test', 'test_deterministic_float_boundaries', 'test_map_determinism',
        't81_determinism_containers_test', 't81_determinism_codec_test',
        't81_determinism_primitives_test', 't81_determinism_float_test',
        't81_determinism_math_test', 't81_determinism_vector_test',
        't81_determinism_new_types_test', 'test_t81float_soft_math',
        't81_ternary_arith_test', 't81_canonfs_driver_test',
        'canonfs_read_verify_test', 'canonfs_read_verify_env_contract_test',
        'canonfs_integrity_matrix_test', 'canonfs_identity_contract_test']
VM = ['t81_vm_determinism_property_test', 't81_vm_deterministic_fault_test',
      't81_vm_fault_family_determinism_matrix_test',
      't81_vm_tloadhash_decodefault_determinism_matrix_test',
      't81_axion_log_determinism_test', 't81_vm_system_registers_deterministic_test',
      't81_isa_binary_io_determinism_test', 't81_vm_workload_determinism_tiers_test',
      'test_resource_monitoring', 't81_determinism_evidence_test']
GUARDS = ['t81_release_'+kind+'_guard' for kind in ('sha3', 'float', 'map')]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source', required=True, type=Path)
    p.add_argument('--expected-source', required=True)
    p.add_argument('--system', choices=['Linux', 'Windows'], required=True)
    p.add_argument('--config', choices=['Debug', 'Release'], required=True)
    p.add_argument('--cc', required=True)
    p.add_argument('--cxx', required=True)
    p.add_argument('--output', required=True, type=Path)
    a = p.parse_args()
    source = a.source.resolve()
    out = a.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    build = out / 'build'
    suffix = '.exe' if os.name == 'nt' else ''
    report = {'expected_source': a.expected_source, 'system_expected': a.system,
              'configuration': a.config, 'platform': platform.platform(),
              'uname': list(platform.uname()), 'python': sys.version,
              'commands': [], 'native_tests': [], 'release_guards': [],
              'mutant_rejections': [], 'portability_regressions': [], 'performance_samples': [],
              'measurement_authorized': False}

    def save():
        (out/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    def run(label, argv, cwd=source, expected=0, timeout=1200):
        argv = [str(x) for x in argv]
        entry = {'label': label, 'argv': argv, 'cwd': str(cwd), 'expected_returncode': expected}
        report['commands'].append(entry)
        print('COMMAND', label, json.dumps(argv), flush=True)
        try:
            child = subprocess.run(argv, cwd=cwd, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, timeout=timeout)
            stdout, stderr = child.stdout, child.stderr
            entry['returncode'] = child.returncode
        except subprocess.TimeoutExpired as exc:
            stdout, stderr = exc.stdout or b'', exc.stderr or b''
            entry['returncode'] = None
            entry['error'] = 'timeout'
        except OSError as exc:
            stdout, stderr = b'', str(exc).encode('utf-8')
            entry['returncode'] = None
            entry['error'] = type(exc).__name__
        (out/(label+'.stdout')).write_bytes(stdout)
        (out/(label+'.stderr')).write_bytes(stderr)
        (out/(label+'.command.json')).write_text(json.dumps(entry, indent=2)+'\n', encoding='utf-8')
        entry['passed'] = entry['returncode'] == expected
        print('RESULT', label, entry['returncode'], flush=True)
        if not entry['passed']:
            print(stderr.decode('utf-8', errors='replace')[-8000:], flush=True)
        save()
        return entry

    try:
        assert platform.system() == a.system, 'Actual OS differs from requested OS'
        head = run('source-head', ['git', 'rev-parse', 'HEAD'])
        assert head['passed'] and (out/'source-head.stdout').read_text().strip() == a.expected_source
        report['source_commit'] = a.expected_source
        report['source_tracked_status'] = subprocess.check_output(
            ['git', 'status', '--porcelain', '--untracked-files=no'], cwd=source, text=True).strip()
        assert not report['source_tracked_status'], 'Tracked source is dirty before execution'
        for name, argv in [('compiler', [a.cxx, '--version'] if a.system == 'Linux' else [a.cxx]),
                           ('cmake-version', ['cmake', '--version']),
                           ('ninja-version', ['ninja', '--version'])]:
            run(name, argv)
        configure = ['cmake', '-S', source, '-B', build, '-G', 'Ninja',
                     '-DCMAKE_BUILD_TYPE='+a.config, '-DCMAKE_C_COMPILER='+a.cc,
                     '-DCMAKE_CXX_COMPILER='+a.cxx, '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
                     '-DT81_BUILD_BENCHMARKS=OFF', '-DT81_BUILD_TUI=OFF',
                     '-DT81_BUILD_EXAMPLES=OFF', '-DT81_BUILD_FUZZ_TESTS=OFF',
                     '-DT81_ENABLE_TERNARYOS=OFF', '-DT81_ENABLE_DPE=OFF',
                     '-DT81_ENABLE_LLAMA_CPP=OFF', '-DT81_BUILD_DIAGNOSTICS=OFF']
        assert run('configure', configure)['passed'], 'Root configure failed'
        if a.system == 'Windows':
            report['portability_regressions'].append(run('msvc-debug-configuration', [sys.executable, '-B',
                source/'tests/python/check_msvc_debug_configuration.py', '--source', source,
                '--cmake', shutil.which('cmake'), '--compiler', shutil.which(a.cxx), '--generator', 'Ninja']))
        run('build-core', ['cmake', '--build', build, '--verbose', '--parallel', '2', '--target', *CORE, *GUARDS])
        for target in CORE:
            binary = build/(target+suffix)
            if binary.exists():
                report['native_tests'].append(run('test-'+target, [binary], timeout=60))
            else:
                report['native_tests'].append({'label': target, 'passed': False, 'error': 'not-built'})
        for kind in ['sha3', 'float', 'map']:
            binary = build/('t81_release_'+kind+'_guard'+suffix)
            if binary.exists():
                report['release_guards'].append(run('guard-'+kind, [sys.executable, '-B',
                    source/'tests/python/check_release_correctness_guard.py', kind, binary], timeout=30))
            else:
                report['release_guards'].append({'label': kind, 'passed': False, 'error': 'not-built'})
        # Original bug mutations are separate test files, never edits to the source checkout.
        if all((build/(x+suffix)).exists() for x in GUARDS):
            mut = out/'mutants'
            mut.mkdir()
            (mut/'release_correctness_guard.cpp').write_bytes((source/'tests/cpp/release_correctness_guard.cpp').read_bytes())
            for filename in ['test_sha3.cpp', 'test_deterministic_float_boundaries.cpp', 'test_map_determinism.cpp']:
                text = (source/'tests/cpp'/filename).read_text(encoding='utf-8')
                if filename == 'test_sha3.cpp':
                    assert 'std::exit(EXIT_FAILURE);' in text
                    text = text.replace('#include <cstdlib>', '#include <cassert>').replace('std::exit(EXIT_FAILURE);', 'assert(false && "SHA3-512 mismatch");')
                else:
                    assert text.startswith('#undef NDEBUG\n')
                    text = text.removeprefix('#undef NDEBUG\n')
                (mut/filename).write_text(text, encoding='utf-8')
            (mut/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(base01_guard_mutants LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
foreach(kind sha3 float map)
  string(TOUPPER "${kind}" upper)
  add_executable(mutant_${kind} release_correctness_guard.cpp)
  target_compile_definitions(mutant_${kind} PRIVATE NDEBUG T81_DETERMINISTIC T81_RELEASE_GUARD_${upper})
  target_include_directories(mutant_${kind} PRIVATE "${PR_SOURCE}/include")
  target_link_libraries(mutant_${kind} PRIVATE "${CORE_LIBRARY}")
endforeach()
''', encoding='utf-8')
            library = build/('t81_core.lib' if a.system == 'Windows' else 'libt81_core.a')
            mbuild = out/'mutant-build'
            mc = run('mutants-configure', ['cmake', '-S', mut, '-B', mbuild, '-G', 'Ninja',
                      '-DCMAKE_BUILD_TYPE='+a.config, '-DCMAKE_CXX_COMPILER='+a.cxx,
                      '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DPR_SOURCE='+str(source),
                      '-DCORE_LIBRARY='+str(library)])
            if mc['passed'] and run('mutants-build', ['cmake', '--build', mbuild, '--verbose', '--parallel', '2'])['passed']:
                for kind in ['sha3', 'float', 'map']:
                    report['mutant_rejections'].append(run('mutant-rejection-'+kind,
                        [sys.executable, '-B', source/'tests/python/check_release_correctness_guard.py',
                         kind, mbuild/('mutant_'+kind+suffix)], expected=1, timeout=30))
        run('build-vm', ['cmake', '--build', build, '--verbose', '--parallel', '2', '--target', *VM])
        if not (build/('t81_determinism_evidence_test'+suffix)).exists():
            run('build-independent-evidence', ['cmake', '--build', build, '--verbose', '--target', 't81_determinism_evidence_test'])
        for target in VM:
            binary = build/(target+suffix)
            if binary.exists():
                report['native_tests'].append(run('test-'+target, [binary], timeout=60))
            else:
                report['native_tests'].append({'label': target, 'passed': False, 'error': 'not-built'})
        if a.system == 'Windows':
            diag = out/'diagnostics'
            diag.mkdir()
            (diag/'evidence_probe.cpp').write_text(
                '#define main underlying_evidence_main\n'
                '#include "'+(source/'tests/determinism/evidence_collector.cpp').as_posix()+'"\n'
                '#undef main\n#include <iostream>\n#include <exception>\n'
                'int main() { std::setvbuf(stdout, nullptr, _IONBF, 0); '
                'std::setvbuf(stderr, nullptr, _IONBF, 0); '
                'try { return underlying_evidence_main(); } '
                'catch (const std::exception& e) { std::cerr << "UNHANDLED_EXCEPTION: " << e.what() << "\\n"; return 1; } }\n',
                encoding='utf-8')
            (diag/'path_probe.cpp').write_text('''#include "t81/canonfs/canon_driver.hpp"
#include "t81/tracing/canonhash.hpp"
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>
int main() {
  auto root = std::filesystem::temp_directory_path()/"base01-native-path-diagnostic";
  std::filesystem::remove_all(root);
  for (const std::string payload : {"canonfs-read-verify-payload", "canonfs-identity-contract"}) {
    auto driver = t81::canonfs::make_persistent_driver(root);
    auto hash = t81::hash::hash_string(payload).to_string();
    errno = 0;
    auto result = driver->write_object(t81::canonfs::ObjectType::RawBlock,
        std::span<const std::byte>(reinterpret_cast<const std::byte*>(payload.data()), payload.size()));
    const int failure_errno = errno;
    std::cout << "payload=" << payload << " hash=" << hash
              << " contains_angle_bracket=" << (hash.find_first_of("<>") != std::string::npos)
              << " write_success=" << result.has_value() << " errno=" << failure_errno
              << " diagnostic=" << std::strerror(failure_errno) << "\\n";
  }
  std::filesystem::remove_all(root);
}
''', encoding='utf-8')
            (diag/'CMakeLists.txt').write_text('''cmake_minimum_required(VERSION 3.20)
project(base01_native_diagnostics LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
foreach(probe evidence path)
  add_executable(${probe}_probe ${probe}_probe.cpp)
  target_compile_definitions(${probe}_probe PRIVATE NDEBUG T81_DETERMINISTIC)
  target_include_directories(${probe}_probe PRIVATE "${PR_SOURCE}/include")
  target_link_libraries(${probe}_probe PRIVATE "${AXION_LIBRARY}" "${CORE_LIBRARY}")
endforeach()
''', encoding='utf-8')
            d_build = out/'diagnostic-build'
            dconfig = run('diagnostics-configure', ['cmake', '-S', diag, '-B', d_build, '-G', 'Ninja',
                '-DCMAKE_BUILD_TYPE='+a.config, '-DCMAKE_CXX_COMPILER='+a.cxx,
                '-DPR_SOURCE='+str(source), '-DAXION_LIBRARY='+str(build/'t81_axion.lib'),
                '-DCORE_LIBRARY='+str(build/'t81_core.lib')])
            if dconfig['passed'] and run('diagnostics-build', ['cmake', '--build', d_build, '--verbose'])['passed']:
                for probe in ['evidence', 'path']:
                    run('diagnostic-'+probe, [d_build/(probe+'_probe.exe')], timeout=30)
        report['source_tracked_status_after'] = subprocess.check_output(
            ['git', 'status', '--porcelain', '--untracked-files=no'], cwd=source, text=True).strip()
        groups = ['native_tests', 'release_guards', 'mutant_rejections', 'portability_regressions']
        report['counts'] = {g: {'planned': len(report[g]), 'executed': sum('returncode' in x for x in report[g]),
                                     'passed': sum(x['passed'] for x in report[g]),
                                     'failed': sum('returncode' in x and not x['passed'] for x in report[g]),
                                     'blocked': sum('returncode' not in x for x in report[g])} for g in groups}
        report['status'] = ('PASS' if len(report['native_tests']) == 27 and len(report['release_guards']) == 3
                            and len(report['mutant_rejections']) == 3 and not report['source_tracked_status_after']
                            and all(x['passed'] for g in groups for x in report[g]) else 'FAIL_OR_BLOCKED')
    except Exception as exc:
        report['status'] = 'FAIL_OR_BLOCKED'
        report['error'] = str(exc)
    finally:
        for directory in [build, out/'mutant-build']:
            for filename in ['CMakeCache.txt', 'compile_commands.json']:
                path = directory/filename
                if path.exists(): shutil.copyfile(path, out/(directory.name+'-'+filename))
        save()
        files = {str(p.relative_to(out)): hashlib.sha256(p.read_bytes()).hexdigest()
                 for p in out.rglob('*') if p.is_file() and 'build' not in p.relative_to(out).parts
                 and 'mutant-build' not in p.relative_to(out).parts and 'diagnostic-build' not in p.relative_to(out).parts}
        (out/'SHA256SUMS.json').write_text(json.dumps(files, indent=2)+'\n', encoding='utf-8')
        print(json.dumps({'status': report['status'], 'counts': report.get('counts'), 'error': report.get('error')}), flush=True)
    return 0 if report['status'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
