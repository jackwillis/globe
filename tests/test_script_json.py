# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Independent JSON parser checks the actual C++ runner output, including incomplete reports."""
import json
import pathlib
import subprocess
import sys
import tempfile


def read_report(data):
    records = [json.loads(line) for line in data.splitlines()]
    if not data.endswith(b"\n") or not records or records[-1].get("type") != "end":
        raise ValueError("incomplete transcript")
    if records[0].get("schema") != "pe-script/1":
        raise ValueError("unsupported schema")
    return records


with tempfile.TemporaryDirectory() as tmp:
    subprocess.run([sys.argv[1], ".", tmp], check=True)
    raw = (pathlib.Path(tmp) / "script-test.jsonl").read_bytes()
    records = read_report(raw)
    run, *commands, end = records
    assert run["script"] == 'test"\\\nscript'
    assert run["app_version"] == "test-version"
    assert run["platform"] == "linux-host-test"
    assert run["script_checksum"] is None
    assert run["initial_borders"] is True
    assert [c["seq"] for c in commands] == [1, 2, 3, 4]
    assert [c["line"] for c in commands] == [2, 3, 4, 5]
    assert commands[0]["result"] == {"type": "value", "value": True}
    assert commands[1]["result"] == {"type": "ok", "phase": "applied", "changed": True}
    assert commands[2]["evidence"] == "application"
    assert commands[2]["result"] == {"type": "ok", "phase": "completed", "condition": "presentation-settled"}
    assert commands[3]["result"] == {"type": "pass", "observed": False}
    assert end == {"type": "end", "status": "pass", "commands": 4, "assertions_failed": 0, "errors": 0}
    for incomplete in (raw.rsplit(b"\n", 2)[0] + b"\n", raw[:-2], b""):
        try:
            read_report(incomplete)
        except (ValueError, json.JSONDecodeError):
            pass
        else:
            raise AssertionError("incomplete report accepted")
print("independent JSONL schema and incomplete-report checks passed")
