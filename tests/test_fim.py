import json
import subprocess
import tempfile
import unittest
from pathlib import Path


PROJECT_DIR = Path(__file__).resolve().parents[1]
FIM = PROJECT_DIR / "fim"


class FileIntegrityTests(unittest.TestCase):
    def setUp(self):
        self.assertTrue(
            FIM.is_file(),
            "Missing fim executable. Run make before testing.",
        )

        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)

        self.workspace = Path(temporary.name).resolve()
        self.monitored = self.workspace / "files"
        self.monitored.mkdir()

        self.example = self.monitored / "example.txt"
        self.example.write_bytes(b"hello\n")

        self.run_fim(str(self.monitored))

    def run_fim(self, *arguments):
        return subprocess.run(
            [str(FIM), *arguments],
            cwd=self.workspace,
            capture_output=True,
            text=True,
            encoding="utf-8",
            check=True,
        )

    def assert_changes(self, expected):
        result = self.run_fim("--check")

        records = [
            json.loads(line)
            for line in result.stdout.splitlines()
        ]

        self.assertTrue(
            records,
            "Checker returned no JSON records.",
        )

        self.assertEqual(records[:-1], expected)

        self.assertEqual(
            records[-1],
            {"type": "summary", "changes": len(expected)},
        )

    def test_unchanged_file(self):
        self.assert_changes([])

    def test_added_file(self):
        added = self.monitored / "added.txt"
        added.write_bytes(b"new file\n")

        self.assert_changes([
            {
                "type": "change",
                "status": "ADDED",
                "path": "added.txt",
            }
        ])

    def test_same_size_modification(self):
        original_size = self.example.stat().st_size
        self.example.write_bytes(b"world\n")

        self.assertEqual(
            self.example.stat().st_size,
            original_size,
        )

        self.assert_changes([
            {
                "type": "change",
                "status": "MODIFIED",
                "path": "example.txt",
            }
        ])

    def test_deleted_file(self):
        self.example.unlink()

        self.assert_changes([
            {
                "type": "change",
                "status": "DELETED",
                "path": "example.txt",
            }
        ])


if __name__ == "__main__":
    unittest.main()