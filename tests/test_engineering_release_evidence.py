"""Red/green matrix for exact-commit software delivery; no network or SIP."""
import copy
import importlib.util
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location(
    "verify_engineering_release", ROOT / "scripts/verify_engineering_release.py")
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)
SHA = "a" * 40


def success_job(name, *names):
    return {"name": name, "status": "completed", "conclusion": "success",
            "steps": [{"name": value, "status": "completed", "conclusion": "success"}
                      for value in names]}


def fixture():
    runs = []
    for i, (kind, name) in enumerate(gate.RUN_NAMES.items(), 101):
        runs.append({"id": i, "name": name, "event": "push",
                     "head_branch": "main", "head_sha": SHA,
                     "status": "completed", "conclusion": "success",
                     "html_url": f"https://github.com/example/axe/actions/runs/{i}"})
    ids = {name: i for i, name in enumerate(gate.RUN_NAMES, 101)}
    jobs = {
        ids["windows"]: [success_job("build-and-test",
            "Build and test with project script",
            "Verify quality gates (unit, blackbox, integration)")],
        ids["baseline"]: [success_job("Integridade MicroSIP | 15 testes estáticos",
            "Executar testes de integridade (não testes SIP)",
            "Validar aceite de engenharia sem homologacao PBX real",
            "Conferir 231 arquivos da importação original")],
        ids["winui"]: [success_job("build-winui3",
            "Test five WinUI 3 preview feature policies (TDD)",
            "Restore and compile real WinUI 3 project",
            "Publish unpackaged UI-only preview with native bridge")],
        ids["native"]: [
            success_job("msvc-native-sdk (abi)",
                "Build pinned MSVC candidate and run real libre reactor"),
            success_job("msvc-native-sdk (tls-media)",
                "Build pinned MSVC candidate and run real libre reactor",
                "Verify literal package file integrity",
                "Execute 1000 native Baresip SIP loopback connection tests",
                "Publish evidence for 1000 SIP connection tests",
                "Build and verify candidate installer",
                "Publish candidate installer",
                "Package WinUI 3 with actual native SIP host (laboratory only)",
                "Build and verify second Windows installer (WinUI 3 native LAB)",
                "Publish WinUI 3 native LAB installer"),
        ],
    }
    artifacts = {
        ids[kind]: [{"name": name.format(sha=SHA), "expired": False,
                      "size_in_bytes": 4096} for name in names]
        for kind, names in gate.EXPECTED_ARTIFACTS.items()
    }
    return runs, jobs, artifacts, ids


class EngineeringDeliveryGateTests(unittest.TestCase):
    def check(self, runs, jobs, artifacts, native_id=104):
        return gate.verify_evidence(SHA, runs, jobs, artifacts, native_id)

    def test_exact_main_sha_with_eight_gates_is_approved_for_engineering_only(self):
        runs, jobs, artifacts, _ = fixture()
        report = self.check(runs, jobs, artifacts)
        self.assertEqual(report["engineering_delivery"], "approved")
        self.assertEqual(report["completed"], 8)
        self.assertEqual(report["total"], 8)
        self.assertTrue(all(report["gates"].values()))
        self.assertFalse(report["production_approved"])
        self.assertEqual(report["external_live_telephony"], "deferred_not_homologated")

    def test_missing_native_run_cannot_reuse_old_package(self):
        runs, jobs, artifacts, _ = fixture()
        runs[-1]["head_sha"] = "b" * 40
        report = self.check(runs, jobs, artifacts)
        self.assertEqual(report["engineering_delivery"], "blocked")
        self.assertFalse(report["gates"]["sip_loopback_1000"])
        self.assertFalse(report["gates"]["native_installer_acceptance"])

    def test_wrong_branch_and_pull_request_results_not_accepted(self):
        for field, invalid in [("head_branch", "feature/test"), ("event", "pull_request")]:
            with self.subTest(field=field):
                runs, jobs, artifacts, _ = fixture()
                runs[1][field] = invalid
                self.assertEqual(self.check(runs, jobs, artifacts)["engineering_delivery"], "blocked")

    def test_failed_and_pending_runs_block_even_with_artifacts(self):
        for status, conclusion in [("completed", "failure"), ("in_progress", None)]:
            with self.subTest(status=status):
                runs, jobs, artifacts, _ = fixture()
                runs[0]["status"] = status
                runs[0]["conclusion"] = conclusion
                self.assertEqual(self.check(runs, jobs, artifacts)["engineering_delivery"], "blocked")

    def test_expired_or_zero_byte_installer_is_rejected(self):
        for expired, size in [(True, 4096), (False, 0)]:
            with self.subTest(expired=expired, size=size):
                runs, jobs, artifacts, _ = fixture()
                a = next(x for x in artifacts[104] if x["name"] == "AXE-Voice-Native-Candidate-Setup")
                a["expired"] = expired
                a["size_in_bytes"] = size
                self.assertEqual(self.check(runs, jobs, artifacts)["engineering_delivery"], "blocked")

    def test_missing_winui_native_installer_is_rejected(self):
        runs, jobs, artifacts, _ = fixture()
        artifacts[104] = [a for a in artifacts[104]
                          if a["name"] != "AXE-Voice-WinUI3-Native-Lab-Setup"]
        report = self.check(runs, jobs, artifacts)
        self.assertFalse(report["gates"]["winui3_lab_candidate"])

    def test_native_sip_tests_step_must_be_successful(self):
        runs, jobs, artifacts, _ = fixture()
        native_tls = jobs[104][1]
        next(s for s in native_tls["steps"] if s["name"].startswith("Execute 1000"))["conclusion"] = "skipped"
        self.assertFalse(self.check(runs, jobs, artifacts)["gates"]["sip_loopback_1000"])

    def test_security_checks_required(self):
        runs, jobs, artifacts, _ = fixture()
        native_tls = jobs[104][1]
        native_tls["steps"] = [s for s in native_tls["steps"] if
                               s["name"] != "Verify literal package file integrity"]
        self.assertFalse(self.check(runs, jobs, artifacts)["gates"]["credentials_and_security_checks"])

    def test_native_run_id_is_pinned_to_trigger(self):
        runs, jobs, artifacts, _ = fixture()
        report = self.check(runs, jobs, artifacts, native_id=999)
        self.assertEqual(report["engineering_delivery"], "blocked")

    def test_latest_run_same_sha_must_be_green(self):
        runs, jobs, artifacts, _ = fixture()
        failed = copy.deepcopy(runs[0])
        failed["id"] = 999
        failed["conclusion"] = "failure"
        runs.append(failed)
        self.assertEqual(self.check(runs, jobs, artifacts)["engineering_delivery"], "blocked")

    def test_malformed_sha_is_rejected(self):
        runs, jobs, artifacts, _ = fixture()
        with self.assertRaises(ValueError):
            gate.verify_evidence("wrong", runs, jobs, artifacts)

    def test_config_contract_remains_fail_closed(self):
        gate.check_scope(ROOT)


if __name__ == "__main__":
    unittest.main()
