import subprocess
import time
import argparse

from monitor import main as run_check


def main():
    parser = argparse.ArgumentParser(
        description="Repeatedly check file interity and save history."
    )

    parser.add_argument(
        "--interval", type=int,default=10,
        help="Seconds to wait after each check (default: 10).",
    )

    args = parser.parse_args()

    if args.interval <= 0:
        parser.error("--interval must be greater than zero.")

    interval = args.interval

    print(
        f"Monitoring started. Waiting {interval} seconds betweem checks.",
        flush=True
    )

    print("Press Ctrl+C to stop.",flush=True)

    try:
        while True:
            try:
                run_check()
            except subprocess.CalledProcessError as error:
                print("Check failed:", flush=True)
            except(OSError, ValueError) as error:
                print(f"Check failed: {error}", flush=True)

            time.sleep(interval)

    except KeyboardInterrupt:
        print("\nMonitoring stopped.", flush=True)

if __name__ == "__main__":
    main()