# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic A/D/E composition experiment; NOT a Reader pack or public format.

Run: python3 atlas/research/composition_ade.py verify|bench
Only stdlib. The six synthetic packs and publication artifacts live in TemporaryDirectory.
"""
from __future__ import annotations

import hashlib
import itertools
import inspect
import json
import os
from pathlib import Path
import statistics
import sys
import tempfile
import time
import tracemalloc

FORMAT = "mock-geographic-pack/1"
BASE = {"slot": "hydro:middle", "entity": "project:river", "value": "embedded-river", "source": "embedded"}
MAX_BYTES = 16384


def canon(value):
    return (json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True) + "\n").encode("ascii")


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


class Reads:
    def __init__(self):
        self.ops = 0
        self.bytes = 0
        self.enumerations = 0

    def get(self, path):
        # This is the *only* read path. No writes occur in any consumer function.
        self.ops += 1
        raw = Path(path).read_bytes()
        self.bytes += len(raw)
        if len(raw) > MAX_BYTES:
            raise ValueError("oversize")
        return raw


def decode_pack(raw):
    doc = json.loads(raw)
    if not isinstance(doc, dict) or doc.get("format") != FORMAT:
        raise ValueError("unknown-format")
    if not all(isinstance(doc.get(k), str) and doc[k] for k in ("publisher", "name")):
        raise ValueError("invalid-identity")
    if not isinstance(doc.get("version"), int) or doc["version"] < 1:
        raise ValueError("invalid-version")
    if doc.get("reader_min") != 1:
        raise ValueError("incompatible-reader")
    claims = doc.get("claims")
    if not isinstance(claims, list) or not (1 <= len(claims) <= 4):
        raise ValueError("invalid-claims")
    for claim in claims:
        if not isinstance(claim, dict) or not all(isinstance(claim.get(k), str) and claim[k] for k in ("slot", "entity", "value")):
            raise ValueError("invalid-claim")
        if not claim["entity"].startswith(doc["publisher"] + ":"):
            raise ValueError("identity-not-publisher-qualified")
    return doc


def collect(paths, reads, expected=None):
    """Validate bytes, identify equivalent duplicates, reject conflicting identical pack IDs."""
    entries, issues = {}, []
    for path in paths:
        name = Path(path).name
        try:
            raw = reads.get(path)
            if expected is not None:
                descriptor = expected[name]
                if len(raw) != descriptor["size"] or digest(raw) != descriptor["digest"]:
                    raise ValueError("integrity-mismatch")
            doc = decode_pack(raw)
            key = (doc["publisher"], doc["name"], doc["version"])
            entries.setdefault(key, []).append((digest(raw), doc))
        except (OSError, UnicodeError, ValueError, KeyError, TypeError, json.JSONDecodeError) as exc:
            issues.append([name, str(exc) if isinstance(exc, ValueError) else type(exc).__name__])
    valid = []
    for key, group in sorted(entries.items()):
        if len({item[0] for item in group}) != 1:
            issues.append(["/".join(map(str, key)), "duplicate-identity-conflict"])
        else:
            valid.append(group[0][1])
    # Multiple versions of a publisher/name do not silently override one another;
    # this prototype reports ambiguity instead of inventing an upgrade policy.
    families = {}
    for p in valid:
        families.setdefault((p["publisher"], p["name"]), []).append(p)
    accepted = []
    for family, members in sorted(families.items()):
        if len(members) > 1:
            issues.append(["/".join(family), "multiple-versions-no-lineage-policy"])
        else:
            accepted.append(members[0])
    return accepted, sorted(issues)


def resolve(packs):
    """Deliberately small conservative policy: conflicting optional slots fall back."""
    by_slot = {}
    for pack in packs:
        for claim in pack["claims"]:
            by_slot.setdefault(claim["slot"], []).append({
                "entity": claim["entity"], "value": claim["value"],
                "source": pack["publisher"] + "/" + pack["name"],
            })
    selected = {BASE["slot"]: BASE}
    conflicts = []
    for slot, entries in sorted(by_slot.items()):
        ordered = sorted(entries, key=lambda x: (x["source"], x["entity"], x["value"]))
        if len(ordered) != 1:
            conflicts.append({"slot": slot, "candidates": ordered, "status": "ambiguous"})
        else:
            selected[slot] = {"slot": slot, **ordered[0]}
    # A conflict at an embedded slot must never erase the embedded fallback.
    return {"selected": [selected[k] for k in sorted(selected)], "conflicts": conflicts}


def scan(paths, reads):  # A: discover files from a folder or inject an enumeration order
    if isinstance(paths, (str, Path)) and Path(paths).is_dir():
        reads.enumerations += 1
        paths = sorted(Path(paths).glob("*.mockpack"))
    packs, issues = collect(paths, reads)
    return resolve(packs), issues


def make_manifest(paths):  # E: publisher / user selects active paths explicitly
    return canon({"format": "mock-active-set/1", "packs": [
        {"file": Path(p).name, "size": len(Path(p).read_bytes()),
         "digest": digest(Path(p).read_bytes())} for p in paths]})


def select_manifest(folder, raw, reads):  # E consumer: flat, explicitly selected set
    try:
        manifest = json.loads(raw)
        if manifest.get("format") != "mock-active-set/1" or not isinstance(manifest.get("packs"), list):
            raise ValueError("invalid-manifest")
        entries = manifest["packs"]
        if any(not isinstance(e, dict) or set(e) != {"file", "size", "digest"}
               or not isinstance(e["file"], str) or Path(e["file"]).name != e["file"]
               or not e["file"].endswith(".mockpack") or not isinstance(e["size"], int)
               or not isinstance(e["digest"], str) for e in entries):
            raise ValueError("invalid-descriptor")
        if len({e["file"] for e in entries}) != len(entries):
            raise ValueError("duplicate-manifest-path")
        expected = {e["file"]: e for e in entries}
        packs, issues = collect([folder / e["file"] for e in entries], reads, expected)
        return resolve(packs), issues
    except (ValueError, TypeError, AttributeError, json.JSONDecodeError) as exc:
        return resolve([]), [["manifest", "invalid-manifest:" + type(exc).__name__]]


def publish_sealed(paths):  # D publisher: precompose; never a Reader action
    reads = Reads()
    packs, issues = collect(paths, reads)
    if issues:
        raise ValueError("publication-refused:" + repr(issues))
    return canon({"format": "mock-sealed/1", "atlas": resolve(packs)})


def read_sealed(raw):  # D consumer: only check the sealed object's shape
    try:
        obj = json.loads(raw)
        if obj.get("format") != "mock-sealed/1" or not isinstance(obj.get("atlas"), dict):
            raise ValueError("bad-seal")
        return obj["atlas"], []
    except (ValueError, TypeError, AttributeError, json.JSONDecodeError):
        return resolve([]), [["sealed", "invalid-seal"]]


def fixture(folder):
    # Six invented resources, distinct publisher-qualified entities even if local IDs agree.
    spec = [
        ("alpha.example", "terrain", "relief:north", "place", "hill"),
        ("beta.example", "terrain", "relief:west", "place", "plain"),
        ("alpha.example", "river", "hydro:middle", "river", "line-A"),
        ("beta.example", "river", "hydro:middle", "river", "line-B"),
        ("gamma.example", "cities", "places:center", "shared", "town"),
        ("delta.example", "cities", "places:south", "shared", "village"),
    ]
    paths = []
    for i, (publisher, name, slot, entity, value) in enumerate(spec):
        doc = {"format": FORMAT, "publisher": publisher, "name": name, "version": 1,
               "reader_min": 1, "claims": [{"slot": slot, "entity": publisher + ":" + entity, "value": value}]}
        path = folder / ("pack-%02d.mockpack" % i)
        path.write_bytes(canon(doc))
        paths.append(path)
    return paths


def verify():
    with tempfile.TemporaryDirectory() as tmp:
        folder = Path(tmp)
        paths = fixture(folder)
        manifest_path, sealed_path = folder / "ACTIVE.mockmanifest", folder / "SEALED.mockatlas"
        manifest_path.write_bytes(make_manifest(paths))
        sealed_path.write_bytes(publish_sealed(paths))
        # Read-only mode: no consumer function writes; disk files also have read-only permissions.
        for path in [*paths, manifest_path, sealed_path]:
            path.chmod(0o444)
        folder.chmod(0o555)
        try:
            a_hashes, e_hashes, d_hashes = set(), set(), set()
            for perm in itertools.permutations(paths):
                a, ai = scan(perm, Reads())
                m = make_manifest(perm)  # Publisher-side permutation, not a consumer disk write.
                e, ei = select_manifest(folder, m, Reads())
                d = publish_sealed(perm)
                assert not ai and not ei
                a_hashes.add(digest(canon(a)))
                e_hashes.add(digest(canon(e)))
                d_hashes.add(digest(d))
            assert len(a_hashes) == len(e_hashes) == len(d_hashes) == 1
            a, issues = scan(paths, Reads())
            e, mi = select_manifest(folder, manifest_path.read_bytes(), Reads())
            d, di = read_sealed(sealed_path.read_bytes())
            assert not issues and not mi and not di and a == e == d
            assert len(a["conflicts"]) == 1 and a["conflicts"][0]["slot"] == "hydro:middle"
            assert any(c["entity"] == "gamma.example:shared" for c in a["selected"])
            assert any(c["entity"] == "delta.example:shared" for c in a["selected"])
            before = sorted((p.name, digest(p.read_bytes())) for p in folder.iterdir())
            scan(paths, Reads()); select_manifest(folder, manifest_path.read_bytes(), Reads()); read_sealed(sealed_path.read_bytes())
            after = sorted((p.name, digest(p.read_bytes())) for p in folder.iterdir())
            assert before == after
            return {"fixture_count": 6, "permutations": 720, "unique_results": {"A": len(a_hashes), "D": len(d_hashes), "E": len(e_hashes)},
                    "snapshot_sha256": digest(canon(a)), "explicit_conflicts": len(a["conflicts"]), "read_only_unchanged": before == after}
        finally:
            folder.chmod(0o755)
            for path in folder.iterdir():
                path.chmod(0o644)


def bench(iterations=80):
    with tempfile.TemporaryDirectory() as tmp:
        folder = Path(tmp)
        paths = fixture(folder)
        manifest = folder / "ACTIVE.mockmanifest"
        sealed = folder / "SEALED.mockatlas"
        manifest.write_bytes(make_manifest(paths))
        sealed.write_bytes(publish_sealed(paths))
        output = {}
        for variant in ("A", "D", "E"):
            timings, peaks, ops, bytes_read, enumerations = [], [], [], [], []
            for _ in range(iterations):
                reads = Reads()
                tracemalloc.start()
                t = time.perf_counter_ns()
                if variant == "A":
                    snap, issues = scan(folder, reads)
                elif variant == "E":
                    snap, issues = select_manifest(folder, reads.get(manifest), reads)
                else:
                    snap, issues = read_sealed(reads.get(sealed))
                elapsed = time.perf_counter_ns() - t
                _, peak = tracemalloc.get_traced_memory()
                tracemalloc.stop()
                assert not issues and len(snap["conflicts"]) == 1
                timings.append(elapsed / 1e6)
                peaks.append(peak)
                ops.append(reads.ops)
                bytes_read.append(reads.bytes)
                enumerations.append(reads.enumerations)
            output[variant] = {"runs": iterations, "median_ms": round(statistics.median(timings), 4),
                               "median_python_alloc_peak_bytes": int(statistics.median(peaks)),
                               "read_ops": ops[0], "read_bytes": bytes_read[0],
                               "directory_enumerations": enumerations[0]}
        # Lines for named routines, NOT a total maintenance-cost estimate.
        output["routine_source_lines"] = {
            "shared": sum(len(inspect.getsourcelines(f)[0]) for f in (decode_pack, collect, resolve)),
            "A": len(inspect.getsourcelines(scan)[0]),
            "D": sum(len(inspect.getsourcelines(f)[0]) for f in (publish_sealed, read_sealed)),
            "E": sum(len(inspect.getsourcelines(f)[0]) for f in (make_manifest, select_manifest)),
        }
        output["scope"] = "Python 3 host warm-page-cache microbenchmark, not OS 9 launch or G4 I/O"
        return output


if __name__ == "__main__":
    command = sys.argv[1] if len(sys.argv) > 1 else "verify"
    if command == "verify":
        print(json.dumps(verify(), indent=2, sort_keys=True))
    elif command == "bench":
        print(json.dumps(bench(), indent=2, sort_keys=True))
    else:
        raise SystemExit("usage: composition_ade.py verify|bench")
