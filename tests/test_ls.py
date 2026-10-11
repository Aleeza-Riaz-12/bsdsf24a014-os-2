"""Regression tests for PA-02's final v1.6.0 binary. Run from root: make test."""
import os
import pathlib
import re
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXECUTABLE = ROOT / 'bin' / 'ls'
ESCAPE = re.compile(r'\x1b\[[0-9;]*m')


def stripped(s):
    return ESCAPE.sub('', s)


class TestLS(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='pa02-test-')
        self.addCleanup(self.temp.cleanup)
        self.root = pathlib.Path(self.temp.name)
        for filename in ('alpha', 'bravo', 'charlie', 'delta', 'echo', 'foxtrot'):
            (self.root / filename).write_text(filename, encoding='utf-8')
        (self.root / '.hidden').write_text('secret', encoding='utf-8')
        (self.root / 'sub').mkdir()
        (self.root / 'sub' / 'child.txt').write_text('child', encoding='utf-8')
        (self.root / 'sub' / 'nested').mkdir()
        (self.root / 'sub' / 'nested' / 'deep.txt').write_text('deep', encoding='utf-8')
        script = self.root / 'run.sh'
        script.write_text('#!/bin/sh\nexit 0\n', encoding='utf-8')
        script.chmod(0o755)
        (self.root / 'bundle.tar').write_bytes(b'test')
        (self.root / 'shortcut').symlink_to('sub', target_is_directory=True)

    def run_ls(self, *args, columns=35):
        env = os.environ.copy()
        env['COLUMNS'] = str(columns)
        return subprocess.run(
            [str(EXECUTABLE), *args, str(self.root)],
            text=True, capture_output=True, env=env, timeout=5,
        )

    def test_plain_listing_is_sorted_and_hides_dotfiles(self):
        result = self.run_ls(columns=14)
        self.assertEqual(result.returncode, 0, result.stderr)
        words = stripped(result.stdout).split()
        self.assertEqual(words, sorted(words))
        self.assertNotIn('.hidden', words)
        self.assertIn('alpha', words)
        self.assertIn('sub', words)

    def test_vertical_prints_down_then_across(self):
        result = self.run_ls(columns=25)
        self.assertEqual(result.returncode, 0, result.stderr)
        rows = [stripped(x).split() for x in result.stdout.splitlines() if x.strip()]
        allnames = sorted(stripped(result.stdout).split())
        width = max(len(x) for x in allnames) + 2
        cols = max(1, min(len(allnames), 25 // width))
        count_rows = (len(allnames) + cols - 1) // cols
        self.assertEqual(rows[0], [allnames[i * count_rows] for i in range(cols) if i * count_rows < len(allnames)])

    def test_horizontal_prints_across_then_down(self):
        result = self.run_ls('-x', columns=25)
        self.assertEqual(result.returncode, 0, result.stderr)
        rows = [stripped(x).split() for x in result.stdout.splitlines() if x.strip()]
        allnames = sorted(stripped(result.stdout).split())
        width = max(len(x) for x in allnames) + 2
        cols = max(1, min(len(allnames), 25 // width))
        self.assertEqual(rows[0], allnames[:cols])

    def test_long_listing_displays_metadata(self):
        result = self.run_ls('-l')
        self.assertEqual(result.returncode, 0, result.stderr)
        output = stripped(result.stdout)
        self.assertRegex(output, r'(?m)^total \d+$')
        self.assertRegex(output, r'(?m)^drwx[^\n]* sub$')
        self.assertRegex(output, r'(?m)^lrwx[^\n]* shortcut -> sub$')
        self.assertRegex(output, r'(?m)^-rwx[^\n]* run\.sh$')
        long_lines = [line for line in output.splitlines() if line and not line.startswith('total ')]
        self.assertEqual([l.split()[-1] for l in long_lines if ' -> ' not in l],
                         sorted([l.split()[-1] for l in long_lines if ' -> ' not in l]))

    def test_colors(self):
        out = self.run_ls('-l').stdout
        self.assertRegex(out, '\x1b\\[1;34msub\x1b\\[0m')
        self.assertRegex(out, '\x1b\\[1;32mrun\\.sh\x1b\\[0m')
        self.assertRegex(out, '\x1b\\[1;31mbundle\\.tar\x1b\\[0m')
        self.assertRegex(out, '\x1b\\[1;35mshortcut\x1b\\[0m')

    def test_recursive_directory_walk_and_no_symlink_follow(self):
        out = stripped(self.run_ls('-R').stdout)
        self.assertIn(f'{self.root}:\n', out)
        self.assertIn(f'{self.root / "sub"}:\n', out)
        self.assertIn(f'{self.root / "sub" / "nested"}:\n', out)
        self.assertIn('child.txt', out)
        self.assertIn('deep.txt', out)
        self.assertNotIn(f'{self.root / "shortcut"}:', out)

    def test_combined_flags(self):
        output = stripped(self.run_ls('-lR').stdout)
        self.assertIn('child.txt', output)
        self.assertRegex(output, r'(?m)^drwx')

    def test_missing_directory_exits_nonzero(self):
        env = os.environ.copy()
        result = subprocess.run([str(EXECUTABLE), str(self.root / 'does-not-exist')],
                                text=True, capture_output=True, env=env)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn('cannot open directory', result.stderr)

    def test_no_follow_symlink_on_recursion(self):
        (self.root / 'loop').symlink_to('.', target_is_directory=True)
        result = self.run_ls('-R')
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(stripped(result.stdout).count(f'{self.root}:\n'), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
