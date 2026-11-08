"""Static contract for the standalone AXE Voice Windows acceptance form."""
import json
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
FORM = ROOT / "docs/qa/AXE-Voice-Windows-QA-Offline-v1.1.html"


class WindowsQAFormContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.html = FORM.read_text(encoding="utf-8")
        match = re.search(r"const CHECKLIST = (\{.*?\});\s*const STORAGE_KEY", cls.html, re.S)
        if match is None:
            raise AssertionError("No offline checklist data")
        cls.checklist = json.loads(match.group(1))

    def test_all_44_cases_and_six_categories_have_unique_ids(self):
        cases = self.checklist["tests"]
        self.assertEqual(len(cases), 44)
        self.assertEqual(sum(t["required"] for t in cases), 39)
        self.assertEqual(len(self.checklist["sections"]), 6)
        self.assertEqual(len({t["id"] for t in cases}), len(cases))
        self.assertEqual({t["group"] for t in cases}, set("ABCDEF"))
        self.assertTrue(all(t["steps"] and t["expected"] for t in cases))

    def test_live_telephony_remains_excluded_from_test_cases(self):
        titles = " ".join(t["title"].lower() for t in self.checklist["tests"])
        for forbidden in ("chamadas reais", "registro sip real", "pabx dígitro", "rtp bidirecional"):
            self.assertNotIn(forbidden, titles)
        self.assertIn("deferred_not_homologated", self.html)
        self.assertIn("productionApproved:false", self.html)

    def test_offline_only_and_no_external_scripts(self):
        self.assertIn("<!DOCTYPE html>", self.html)
        self.assertNotRegex(self.html, r"<script\s+[^>]*src\s*=")
        self.assertNotRegex(self.html, r"<iframe\b")
        for token in ("fetch(", "XMLHttpRequest", "WebSocket(", "navigator.sendBeacon"):
            self.assertNotIn(token, self.html)
        self.assertIn("function exportJson()", self.html)
        self.assertIn("function exportCsv()", self.html)
        self.assertIn("function exportMarkdown()", self.html)
        self.assertIn("function githubIssueMarkdown()", self.html)

    def test_import_validation_and_csv_formula_protection(self):
        self.assertIn("function normalize(value)", self.html)
        self.assertIn("typeof value!=='object'", self.html)
        self.assertIn("function csvCell(raw)", self.html)
        self.assertIn("if(/^\\s*[=+@-]/.test(s))", self.html)
        self.assertIn("function safeMarkdown(raw)", self.html)
        self.assertIn("file.size>2_000_000", self.html)

    def test_local_acceptance_rejects_missing_or_failed_required_items(self):
        self.assertIn("function evaluateLocalVerdict()", self.html)
        self.assertIn("missing.length===0 && problems.length===0", self.html)
        self.assertIn("localAcceptanceVerified:evaluateLocalVerdict().valid", self.html)
        self.assertIn("function updateTriageView()", self.html)

    def test_credentials_are_never_requested(self):
        inputs = re.findall(r"<input[^>]+(?:id|name)=\"([^\"]+)\"", self.html)
        for name in inputs:
            self.assertNotIn("password", name.lower())
            self.assertNotIn("senha", name.lower())


if __name__ == "__main__":
    unittest.main()
