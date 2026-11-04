"""Host-only checks. Requires Python 3, g++ and Node.js; no board required."""
import os
import subprocess
import tempfile
from pathlib import Path

base = Path(__file__).resolve().parent.parent
env = os.environ.copy()
env['ASAN_OPTIONS'] = 'detect_leaks=0'
with tempfile.TemporaryDirectory(prefix='nexus611-') as temp:
    flags = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', '-Wno-unused-parameter', '-I', 'tests/stubs']
    for name in ['test_nexus_6_1_1', 'test_activity_6_1_1', 'test_watch_navigation_611', 'test_audit_611', 'test_battery_profile_611', 'test_studio_protocol611', 'test_osd611', 'test_display_storage611', 'test_faces630', 'test_power670','test_pm690','test_pm_uart691','test_render692','test_energy693']:
        exe = str(Path(temp) / name)
        subprocess.run(flags + ['-fsanitize=address,undefined', 'tests/' + name + '.cpp', '-o', exe], cwd=base, check=True)
        subprocess.run([exe], cwd=base, env=env, check=True)
    bridge = str(Path(temp) / 'studio_bridge611')
    subprocess.run(flags + ['tests/studio_bridge611.cpp', '-o', bridge], cwd=base, check=True)
    for scenario in [None, '--unformatted', '--no-partition', '--low-space']:
        subprocess.run(['node', 'tests/test_studio_app611.js', bridge] + ([scenario] if scenario else []), cwd=base, check=True)
print('All host checks passed. Arduino build, real browser and physical hardware still require verification.')
