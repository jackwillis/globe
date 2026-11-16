# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic conformance tests for research-only A/D/E comparison."""
import json
from pathlib import Path
import tempfile
import unittest

from composition_ade import (FORMAT, Reads, canon, collect, decode_pack, digest,
                             fixture, make_manifest, publish_sealed, read_sealed,
                             resolve, scan, select_manifest, verify)


class CompositionTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.folder = Path(self.tmp.name)
        self.paths = fixture(self.folder)

    def test_720_permutations_and_read_only(self):
        result = verify()
        self.assertEqual(result["permutations"], 720)
        self.assertTrue(result["read_only_unchanged"])

    def test_three_designs_same_active_set(self):
        a, ai = scan(self.paths, Reads())
        e, ei = select_manifest(self.folder, make_manifest(self.paths), Reads())
        d, di = read_sealed(publish_sealed(self.paths))
        self.assertEqual((ai, ei, di), ([], [], []))
        self.assertEqual(a, e)
        self.assertEqual(e, d)
        self.assertEqual(a["selected"][0]["source"], "embedded")
        self.assertEqual(len(a["conflicts"]), 1)

    def test_directory_discovery_agrees_with_supplied_inventory(self):
        discovered, issues = scan(self.folder, Reads())
        injected, other_issues = scan(list(reversed(self.paths)), Reads())
        self.assertEqual((issues, other_issues), ([], []))
        self.assertEqual(discovered, injected)

    def test_manifest_explicit_selection_where_scan_cannot(self):
        full, _ = scan(self.paths, Reads())
        selected, problems = select_manifest(self.folder, make_manifest(self.paths[:3] + self.paths[4:]), Reads())
        self.assertEqual(problems, [])
        self.assertEqual(len(full["conflicts"]), 1)
        self.assertEqual(len(selected["conflicts"]), 0)
        self.assertEqual(next(x for x in selected["selected"] if x["slot"] == "hydro:middle")["value"], "line-A")
        # D can produce that same selection, but requires republishing a new sealed atlas.
        published, _ = read_sealed(publish_sealed(self.paths[:3] + self.paths[4:]))
        self.assertEqual(published, selected)

    def test_missing_pack_explicit_and_embedded_safe(self):
        manifest = make_manifest(self.paths)
        self.paths[0].unlink()
        selected, problems = select_manifest(self.folder, manifest, Reads())
        self.assertTrue(any(p[0] == "pack-00.mockpack" for p in problems))
        self.assertTrue(any(x["source"] == "embedded" for x in selected["selected"]))

    def test_corrupted_data_a_and_e_report_failures(self):
        manifest = make_manifest(self.paths)
        self.paths[1].write_bytes(b'{"incomplete":')
        _, a_problems = scan(self.paths, Reads())
        _, e_problems = select_manifest(self.folder, manifest, Reads())
        self.assertTrue(any(x[0] == "pack-01.mockpack" for x in a_problems))
        self.assertTrue(any(x[1] == "integrity-mismatch" for x in e_problems))

    def test_well_formed_mutation_detected_by_manifest_not_scan(self):
        manifest = make_manifest(self.paths)
        data = json.loads(self.paths[0].read_bytes())
        data["claims"][0]["value"] = "changed-after-publication"
        self.paths[0].write_bytes(canon(data))
        _, a_issues = scan(self.paths, Reads())
        _, e_issues = select_manifest(self.folder, manifest, Reads())
        self.assertEqual(a_issues, [])  # A has no independent expected digest.
        self.assertTrue(any(x[1] == "integrity-mismatch" for x in e_issues))

    def test_incompatible_reader_and_publisher_scoped_identity(self):
        data = json.loads(self.paths[4].read_bytes())
        self.assertNotEqual(data["claims"][0]["entity"], json.loads(self.paths[5].read_bytes())["claims"][0]["entity"])
        data["reader_min"] = 2
        self.paths[4].write_bytes(canon(data))
        selected, issues = scan(self.paths, Reads())
        self.assertTrue(any(x[1] == "incompatible-reader" for x in issues))
        self.assertNotIn("places:center", [x["slot"] for x in selected["selected"]])

    def test_invalid_entity_namespace(self):
        data = json.loads(self.paths[4].read_bytes())
        data["claims"][0]["entity"] = "project:unauthorized"
        with self.assertRaisesRegex(ValueError, "identity-not-publisher-qualified"):
            decode_pack(canon(data))

    def test_duplicate_same_identity_conflicting_bytes_is_explicit(self):
        other = self.folder / "pack-99.mockpack"
        data = json.loads(self.paths[0].read_bytes())
        data["claims"][0]["value"] = "different-publisher-artifact"
        other.write_bytes(canon(data))
        selected, issues = scan(self.paths + [other], Reads())
        self.assertTrue(any(x[1] == "duplicate-identity-conflict" for x in issues))
        self.assertNotIn("relief:north", [x["slot"] for x in selected["selected"]])
        with self.assertRaisesRegex(ValueError, "publication-refused"):
            publish_sealed(self.paths + [other])

    def test_byte_identical_duplicate_is_idempotent(self):
        dup = self.folder / "pack-99.mockpack"
        dup.write_bytes(self.paths[0].read_bytes())
        first, _ = scan(self.paths, Reads())
        again, issues = scan(self.paths + [dup], Reads())
        self.assertEqual(issues, [])
        self.assertEqual(first, again)

    def test_multiple_versions_are_not_silently_upgraded(self):
        new = self.folder / "pack-99.mockpack"
        data = json.loads(self.paths[0].read_bytes())
        data["version"] = 2
        new.write_bytes(canon(data))
        selected, issues = scan(self.paths + [new], Reads())
        self.assertTrue(any(x[1] == "multiple-versions-no-lineage-policy" for x in issues))
        self.assertNotIn("relief:north", [x["slot"] for x in selected["selected"]])

    def test_invalid_manifest_and_seal_fallback(self):
        atlas, issues = select_manifest(self.folder, b'{"format":"unknown"}', Reads())
        self.assertEqual(atlas, resolve([]))
        self.assertTrue(issues)
        atlas, issues = read_sealed(b'broken')
        self.assertEqual(atlas, resolve([]))
        self.assertTrue(issues)

    def test_well_formed_sealed_mutation_is_detected(self):
        seal = json.loads(publish_sealed(self.paths))
        seal["atlas"]["selected"][0]["value"] = "tampered"
        fallback, issues = read_sealed(canon(seal))
        self.assertEqual(fallback, resolve([]))
        self.assertTrue(any(x[1] == "integrity-mismatch" for x in issues))

    def test_no_unrequested_disk_artifacts_from_consumer(self):
        manifest = make_manifest(self.paths)
        sealed = publish_sealed(self.paths)
        before = sorted(p.name for p in self.folder.iterdir())
        scan(self.paths, Reads())
        select_manifest(self.folder, manifest, Reads())
        read_sealed(sealed)
        self.assertEqual(before, sorted(p.name for p in self.folder.iterdir()))


if __name__ == "__main__":
    unittest.main()
