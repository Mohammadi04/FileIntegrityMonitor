import json
import subprocess
from datetime import datetime, timezone
from pathlib import Path


def main():
    project_dir = Path(__file__).resolve().parent

    result = subprocess.run(
        [str(project_dir/ "fim"), "--check"],
        cwd=project_dir, capture_output=True,
        text=True, encoding="utf-8", check=True,
    )

    records = [json.loads(line)
               for line in result.stdout.splitlines()]

    if not records or any(
        not isinstance(record, dict) for record in records
    ):
        raise ValueError("Checker returned missing or invalid records.")

    summary = records[-1]
    changes = records[:-1]

    if(
        summary.get("type") != "summary"
        or type(summary.get("changes")) is not int
        or summary["changes"] != len(changes)
    ):
        raise ValueError("Missing or inconsistent check summary. ")

    for change in changes:
        if(
            change.get("type") != "change"
            or change.get("status") not in {"ADDED", "MODIFIED", "DELETED"}
            or not isinstance(change.get("path"), str)
            or not change["path"]
        ):
            raise ValueError("Invalid change record.")

    history_record = {
        "Checked_at": datetime.now(timezone.utc).isoformat(),
        "Change_count":summary["changes"],
        "changes":changes,
    }

    history_path = project_dir/"history.jsonl"

    with history_path.open("a", encoding="utf-8") as history:
        history.write(json.dumps(history_record) + "\n")

    print(f"Check saved. Changes found: {summary['changes']}")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as error:
        print("C++ checker failed")
        print(error.stderr.strip())
        raise SystemExit(1)
    except(OSError, ValueError) as error:
        print(f"Error: {error}")
        raise SystemExit(1)
