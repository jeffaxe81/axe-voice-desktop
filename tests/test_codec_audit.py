"""Offline, synthetic binary-header tests. No 3rd-party binaries required."""
from pathlib import Path
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from codec_audit import audit, binary_arch, candidates_only_config, main  # noqa: E402


def fake_pe(path: Path, machine: bytes = b"\x64\x86") -> None:
    data = bytearray(128)
    data[0:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 64)
    data[64:70] = b"PE\x00\x00" + machine
    path.write_bytes(data)


def fake_elf(path: Path, machine: int = 62) -> None:
    data = bytearray(64)
    data[:6] = b"\x7fELF\x02\x01"
    data[18:20] = machine.to_bytes(2, "little")
    path.write_bytes(data)


class CodecAuditTests(unittest.TestCase):
    def setUp(self) -> None:
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.mods = self.root / "modules"
        self.mods.mkdir()
        self.libs = self.root / "libs"
        self.libs.mkdir()

    def state(self, report: dict, codec: str) -> str:
        return next(x["state"] for x in report["codecs"] if x["codec"] == codec)

    def test_empty_directory_is_not_homologated(self):
        r = audit(self.mods, [self.libs], "windows-x64")
        self.assertEqual(r["basic_profile_file_candidates"], 0)
        self.assertFalse(r["live_media_verified"])
        self.assertFalse(r["commercial_distribution_approved"])
        self.assertTrue(all(not c["homologated"] for c in r["codecs"]))

    def test_windows_basic_candidates(self):
        for stem in ("g711", "g722", "opus"):
            fake_pe(self.mods / f"{stem}.dll")
        fake_pe(self.libs / "libopus.dll")
        r = audit(self.mods, [self.libs], "windows-x64")
        self.assertEqual(r["basic_profile_file_candidates"], 3)
        self.assertEqual(self.state(r, "Opus"), "candidate-only")

    def test_missing_opus_dependency(self):
        fake_pe(self.mods / "opus.dll")
        self.assertEqual(self.state(audit(self.mods, [], "windows-x64"), "Opus"), "dependency-missing")

    def test_dependency_may_share_module_basename_in_separate_dir(self):
        fake_pe(self.mods / "opus.dll")
        fake_pe(self.libs / "opus.dll")
        self.assertEqual(self.state(audit(self.mods, [self.libs], "windows-x64"), "Opus"), "candidate-only")

    def test_wrong_pe_arch_rejected(self):
        fake_pe(self.mods / "g711.dll", machine=b"\x4c\x01")
        self.assertEqual(binary_arch(self.mods / "g711.dll"), "other-pe")
        self.assertEqual(self.state(audit(self.mods, [], "windows-x64"), "PCMA/PCMU (G.711)"), "module-architecture-mismatch")

    def test_linux_elf64_accepted(self):
        fake_elf(self.mods / "g711.so")
        r = audit(self.mods, [], "linux-x64")
        self.assertEqual(self.state(r, "PCMA/PCMU (G.711)"), "candidate-only")

    def test_linux_other_machine_rejected(self):
        fake_elf(self.mods / "g711.so", machine=183)
        self.assertEqual(binary_arch(self.mods / "g711.so"), "other-elf")

    def test_g729_never_auto_approved(self):
        fake_pe(self.mods / "g729.dll")
        fake_pe(self.libs / "libbcg729.dll")
        r = audit(self.mods, [self.libs], "windows-x64")
        self.assertEqual(self.state(r, "G.729"), "manual-license-review-required")
        self.assertFalse(r["commercial_distribution_approved"])

    def test_speex_requires_custom_module_review(self):
        fake_pe(self.mods / "speex.dll")
        fake_pe(self.libs / "libspeex.dll")
        self.assertEqual(self.state(audit(self.mods, [self.libs], "windows-x64"), "Speex"), "custom-module-review-required")

    def test_symlink_modules_ignored(self):
        fake_pe(self.root / "g711.dll")
        try:
            (self.mods / "g711.dll").symlink_to(self.root / "g711.dll")
        except (OSError, NotImplementedError):
            self.skipTest("symlinks unavailable")
        r = audit(self.mods, [], "windows-x64")
        self.assertEqual(self.state(r, "PCMA/PCMU (G.711)"), "module-missing")

    def test_config_comments_cannot_enable_modules(self):
        fake_pe(self.mods / "g711.dll")
        txt = candidates_only_config(audit(self.mods, [], "windows-x64"))
        self.assertIn("# module g711.dll", txt)
        self.assertTrue(all(line.startswith("#") for line in txt.splitlines()))
        self.assertNotIn("# module g729.dll", txt)

    def test_cli_blocker_exit_code(self):
        self.assertEqual(main(["--modules", str(self.mods), "--target", "windows-x64", "--fail-on-missing-basic"]), 2)

    def test_unrecognized_binary(self):
        bad = self.mods / "g711.dll"
        bad.write_text("not an executable", encoding="utf8")
        self.assertEqual(binary_arch(bad), "unrecognized")


if __name__ == "__main__":
    unittest.main()
