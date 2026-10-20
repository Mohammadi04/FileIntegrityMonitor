# File Integrity Monitor

A C++17 and Python tool that detects added, modified, and deleted
files by comparing SHA-256 hashes against a saved baseline.

I built this project to learn filesystem programming, content hashing,
C++–Python integration, and automated testing.

## Features

- Recursively scans a directory and its subdirectories.
- Calculates SHA-256 hashes using an 8 KiB reading buffer.
- Saves file paths and hashes in a persistent baseline.
- Detects content changes even when file size stays the same.
- Produces JSON Lines output for Python.
- Saves timestamped check history.
- Displays the latest five checks.
- Repeats checks with a configurable polling interval.
- Includes four automated integration tests.

## Requirements

- macOS: hashing currently uses Apple's CommonCrypto library.
- A C++17 compiler, such as Apple Clang.
- Make.
- Python 3.9 or newer.

Python scripts use only the standard library.
Homebrew and CMake are not required.

## Build

Clone the repository and compile the C++ checker:

```bash
git clone https://github.com/Mohammadi04/FileIntegrityMonitor.git
cd FileIntegrityMonitor
make
```

This creates the `fim` executable.

## Create a baseline

From the project root, create a baseline for the sample directory:

```bash
./fim test_data
```

The program saves `baseline.txt` in the current working directory.
It records the monitored directory's absolute path and each file's
relative path and SHA-256 hash.

The baseline must be outside the monitored directory. The program
refuses to overwrite an existing baseline.

If you already created a baseline for this directory, skip this step.

A baseline records a reference state; it does not prove that the
original files are safe and does not contain backups of their contents.

## Check for changes

Run the C++ checker directly:

```bash
./fim --check
```

Check mode writes JSON Lines to standard output and scan diagnostics
to standard error.

To run a check and save its results to history:

```bash
python3 monitor.py
```

Each successful check appends a record to `history.jsonl`.
The timestamp records when the check finished, not when a file changed.

## Try a same-size modification

The sample `example.txt` contains `hello` followed by a newline.
Replace it with `world` followed by a newline:

```bash
printf 'world\n' > test_data/example.txt
python3 monitor.py
```

The file remains six bytes, but its hash changes. The monitor should
report one modification.

Restore the original contents:

```bash
printf 'hello\n' > test_data/example.txt
python3 monitor.py
```

If no other files changed, the change count returns to zero.

## View recent history

```bash
python3 report.py
```

The report displays the latest five saved checks, oldest first.
Earlier records remain in the history file.

## Watch repeatedly

```bash
python3 watch.py --interval 10
```

The watcher runs a check immediately, then waits ten seconds after
each check finishes. Press Ctrl+C to stop.

The interval defaults to ten seconds and must be a positive integer.

```bash
python3 watch.py --help
```

## Run tests

```bash
make
python3 -m unittest discover -s tests -v
```

The four integration tests verify:

- Unchanged files.
- Added files.
- Deleted files.
- Modified content with an unchanged file size.

Each test runs the compiled executable using an isolated temporary
directory and baseline.

## Project files

| File | Purpose |
|---|---|
| `src/main.cpp` | Scanning, hashing, baseline storage, and comparison |
| `monitor.py` | Runs the checker, validates output, and saves history |
| `report.py` | Displays recent saved checks |
| `watch.py` | Runs repeated checks with a configurable interval |
| `tests/test_fim.py` | C++ checker integration tests |
| `Makefile` | Builds and cleans the executable |
| `test_data/` | Sample files for demonstrations |

Generated files `fim`, `baseline.txt`, and `history.jsonl` are ignored
by Git.

## How it works

The C++ scanner builds a map of relative file paths to SHA-256 hashes.
During a check, it compares the current map with the saved baseline:

| Condition | Result |
|---|---|
| Path exists only in the current scan | ADDED |
| Path exists in both but hashes differ | MODIFIED |
| Path exists only in the baseline | DELETED |

Python runs the checker as a subprocess, validates its JSON output,
and appends successful checks to history.

## Current limitations

- CommonCrypto makes the current C++ implementation macOS-specific.
- Checks read file contents again each time, which can be costly for
  large directories.
- Polling can miss changes that are made and reversed between scans.
- Files can change during a scan; it is not an atomic filesystem snapshot.
- Symbolic-link entries are skipped.
- A rename is generally reported as a deletion and an addition.
- Repeated checks report persistent differences from the original
  baseline, even if no additional edit occurred.
- Failed checks print errors but are not saved in history.
- The JSON interface assumes valid UTF-8 filenames.
- The baseline and history are local files without tamper protection.
- The tool detects differences; it cannot identify who caused them,
  determine whether they were malicious, or restore original contents.