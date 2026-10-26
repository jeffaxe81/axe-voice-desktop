"""Release metadata must agree without changing any source files."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'scripts/check_release_versions.py'


class ReleaseVersionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.files = {
            'CMakeLists.txt': 'project(AXEVoiceDesktop VERSION 0.58.0 LANGUAGES CXX)\n',
            'installer/axe-native-candidate.iss': '[Setup]\nAppVersion=0.58.0\n',
            'installer/CANDIDATE-README.txt': 'AXE Voice Native Candidate 0.58.0\nDetails\n',
            'src/diagnostic_export.h': r'const auto source=std::wstring(L"AXE Voice candidate 0.58.0\n");',
            'src/main.cpp': 'const wchar_t* title=L"AXE Voice | Candidato nativo 0.58";\n',
        }
        self.write_files()

    def write_files(self):
        for name, content in self.files.items():
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding='utf-8')

    def run_gate(self):
        before = {p: p.read_bytes() for p in self.root.rglob('*') if p.is_file()}
        result = subprocess.run([sys.executable, str(SCRIPT), str(self.root)],
                                capture_output=True, text=True)
        self.assertEqual(before, {p: p.read_bytes() for p in self.root.rglob('*') if p.is_file()})
        return result

    def test_valid_title_may_omit_final_zero(self):
        self.assertEqual(self.run_gate().returncode, 0)

    def test_exact_versions_including_current_release(self):
        for version in ('0.58.0', '0.57.1'):
            with self.subTest(version=version):
                self.files = {name: value.replace('0.58.0', version).replace('nativo 0.58"', f'nativo {version}"')
                              for name, value in self.files.items()}
                self.write_files()
                self.assertEqual(self.run_gate().returncode, 0)

    def test_each_divergence_is_rejected(self):
        for name, content in self.files.items():
            with self.subTest(name=name):
                (self.root / name).write_text(content.replace('0.58', '0.59'), encoding='utf-8')
                result = self.run_gate()
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('version', result.stderr.lower())
                self.write_files()

    def test_each_missing_file_or_marker_is_rejected(self):
        for name in self.files:
            for missing_file in (True, False):
                with self.subTest(name=name, missing_file=missing_file):
                    path = self.root / name
                    if missing_file:
                        path.unlink()
                    else:
                        path.write_text('unrelated text\n', encoding='utf-8')
                    result = self.run_gate()
                    self.assertNotEqual(result.returncode, 0)
                    self.assertIn(name, result.stderr)
                    self.write_files()

    def test_ambiguous_markers_are_rejected(self):
        for name, content in self.files.items():
            if name.endswith('README.txt'):
                continue  # Only the first line is the release marker.
            with self.subTest(name=name):
                (self.root / name).write_text(content + '\n' + content, encoding='utf-8')
                self.assertNotEqual(self.run_gate().returncode, 0)
                self.write_files()

    def test_only_title_may_omit_final_zero(self):
        for name, content in self.files.items():
            if name == 'src/main.cpp':
                continue
            with self.subTest(name=name):
                (self.root / name).write_text(content.replace('0.58.0', '0.58'), encoding='utf-8')
                self.assertNotEqual(self.run_gate().returncode, 0)
                self.write_files()

    def test_title_cannot_omit_nonzero_patch(self):
        self.files = {name: content.replace('0.58.0', '0.58.1') for name, content in self.files.items()}
        self.write_files()
        self.assertNotEqual(self.run_gate().returncode, 0)

    def test_readme_release_must_be_on_first_line(self):
        path = self.root / 'installer/CANDIDATE-README.txt'
        path.write_text('\n' + self.files['installer/CANDIDATE-README.txt'], encoding='utf-8')
        self.assertNotEqual(self.run_gate().returncode, 0)


if __name__ == '__main__':
    unittest.main()
