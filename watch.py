import subprocess
import time


from monitor import main as run_check


def main():
    interval = 10

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