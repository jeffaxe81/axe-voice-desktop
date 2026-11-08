"""Ensure deferred live-PBX approval can never be silently promoted to production."""
import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/check_release_scope.py"


class ReleaseScopeConfigTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.scope = json.loads((ROOT / "config/release-scope.json").read_text(encoding="utf-8"))
        for path in ("src/release_scope_policy.h", "CMakeLists.txt"):
            target = self.root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((ROOT / path).read_bytes())
        self.save()

    def save(self):
        target = self.root / "config/release-scope.json"
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(json.dumps(self.scope, ensure_ascii=False), encoding="utf-8")

    def check(self):
        return subprocess.run([sys.executable, str(SCRIPT), str(self.root)],
                              capture_output=True, text=True)

    def test_real_repository_contract(self):
        result = subprocess.run([sys.executable, str(SCRIPT), str(ROOT)],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_deferred_live_telephony_passes_validation(self):
        self.assertEqual(self.check().returncode, 0)

    def test_production_can_never_be_auto_approved(self):
        self.scope["production"]["approved"] = True
        self.save()
        self.assertNotEqual(self.check().returncode, 0)

    def test_live_pbx_not_required_for_engineering(self):
        self.scope["real_telephony"]["required_for_engineering_delivery"] = True
        self.save()
        self.assertNotEqual(self.check().returncode, 0)

    def test_live_pbx_always_required_for_production(self):
        self.scope["real_telephony"]["required_for_production"] = False
        self.save()
        self.assertNotEqual(self.check().returncode, 0)

    def test_missing_evidence_gate_cannot_be_omitted(self):
        self.scope["engineering_gates"].pop()
        self.save()
        self.assertNotEqual(self.check().returncode, 0)

    def test_synthetic_pass_does_not_allow_claim_of_100_percent(self):
        self.scope["engineering_status"] = "complete"
        self.save()
        self.assertNotEqual(self.check().returncode, 0)

    def test_future_features_remain_excluded(self):
        self.scope["excluded_future_features"].append(239)
        self.save()
        self.assertNotEqual(self.check().returncode, 0)

    def test_version_consistency(self):
        self.scope["release"] = "9.99.99"
        self.save()
        self.assertNotEqual(self.check().returncode, 0)


if __name__ == "__main__":
    unittest.main()
