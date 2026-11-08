"""Execute Node-only acceptance tests for the standalone offline Windows QA dashboard."""
from pathlib import Path
import re
import shutil
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]
DASHBOARD = ROOT / "docs/qa/AXE-Voice-Consolidador-Windows-v1.0.html"
NODE_TEST = ROOT / "tests/qa_dashboard_node_test.cjs"


class WindowsQADashboardTests(unittest.TestCase):
    def test_offline_form_policies_and_no_external_network(self):
        html = DASHBOARD.read_text(encoding="utf-8")
        self.assertIn("<!DOCTYPE html>", html)
        self.assertIn("const CASES=", html)
        self.assertIn("productionApproved:false", html)
        self.assertIn("liveTelephony:", html)
        self.assertIn("Sem PBX real", html) if "Sem PBX real" in html else self.assertIn("sem PBX real", html)
        self.assertNotRegex(html, r'<script\s+[^>]*src\s*=')
        self.assertNotRegex(html, r'<iframe\b')
        for banned in ("fetch(", "XMLHttpRequest", "WebSocket(", "navigator.sendBeacon",
                       "localStorage", "indexedDB", "sessionStorage"):
            self.assertNotIn(banned, html)
        self.assertIn("if(f.size>2_000_000)", html)
        self.assertIn("reports.length+files.length>100", html)
        self.assertIn('function normalized(value,filename)', html)

    def test_node_qa_contract_against_malformed_and_forged_results(self):
        node = shutil.which("node")
        self.assertIsNotNone(node, "Node.js 22 must be provisioned in quality CI")
        result = subprocess.run([node, str(NODE_TEST)], cwd=ROOT,
                                capture_output=True, text=True, timeout=20)
        self.assertEqual(result.returncode, 0,
                         result.stdout + "\n" + result.stderr)
        self.assertIn("PASS", result.stdout)


if __name__ == "__main__":
    unittest.main()
