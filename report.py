# Finds history.jsonl using its own location.
# Reads one record at a time.
# Uses deque(maxlen=5) to keep only the latest five records in memory.
# Reports malformed records with their line numbers.
# Displays timestamps, counts, and changed paths.

import json
from collections import deque
from pathlib import Path


def main():
    project_dir = Path(__file__).resolve().parent
    history_path = project_dir / "history.jsonl"

    recent_checks = deque(maxlen=5)

    with history_path.open("r", encoding="utf-8") as history:
        for line_number, line in enumerate(history, start=1):
            if not line.strip():
                continue

            try:
                record = json.loads(line)

                if not isinstance(record, dict):
                    raise ValueError("Expected a JSON object.")

                if (
                    not isinstance(record.get("checked_at"), str)
                    or type(record.get("change_count")) is not int
                    or not isinstance(record.get("changes"), list)
                ):
                    raise ValueError("Missing or invalid check fields.")

                if record["change_count"] != len(record["changes"]):
                    raise ValueError("Change count does not match details.")

                for change in record["changes"]:
                    if (
                        not isinstance(change, dict)
                        or change.get("type") != "change"
                        or change.get("status")
                        not in {"ADDED", "MODIFIED", "DELETED"}
                        or not isinstance(change.get("path"), str)
                        or not change["path"]
                    ):
                        raise ValueError("Invalid change details.")

            except ValueError as error:
                raise ValueError(
                    f"Invalid history at line {line_number}: {error}"
                ) from error

            recent_checks.append(record)

    if not recent_checks:
        print("No checks recorded yet.")
        return

    print(f"Latest {len(recent_checks)} checks (oldest first):")

    for record in recent_checks:
        print(
            f"\n{record['checked_at']} | "
            f"Changes: {record['change_count']}"
        )

        if not record["changes"]:
            print("  No differences from baseline.")
        else:
            for change in record["changes"]:
                print(
                    f"  {change['status']}: "
                    f"{json.dumps(change['path'])}"
                )


if __name__ == "__main__":
    try:
        main()
    except FileNotFoundError:
        print("No history file found. Run monitor.py first.")
        raise SystemExit(1)
    except (OSError, ValueError) as error:
        print(f"Error: {error}")
        raise SystemExit(1)