# SPDX-License-Identifier: GPL-3.0-or-later
"""Synthetic conformance tests for EXP-EDSEL-01 (predictions P1-P10 in the research note)."""
import itertools
import unittest

from edition_selection import (
    AMBIGUOUS, DETAIL_UNAVAILABLE, ENTITY_UNKNOWN, GHOST, MADISON_CITY, MADISON_IN, MADISON_TOWN,
    MONACO, OK, OLD_COUNTY, OLD_VILLAGE, COUNTY_A, COUNTY_B, REVISION_MISSING, RETIRED,
    SNAPSHOT_DRIFT, SOURCE_MISSING, SPLIT, SPRINGFIELD_W, SPRINGFIELD_WI, SYNTHETIC, UNKNOWN, WI,
    ConfigChanged, Library, Model, Navigate, Resolved, StaleView, decode_ref, encode_ref,
    fixture, flatten_retirements, format_ref, lookup_name, make_config, owned, parse_text,
    query, record_command, replay, resolve_text, run_effect, update)

CAT, SRC = fixture()
W, WS, C = SRC['world'], SRC['wisc'], SRC['census']
ALL_REFS = sorted(CAT.kinds)


def subsets():
    pool = [W, WS, C]
    for n in range(len(pool) + 1):
        yield from itertools.combinations(pool, n)


def configs():
    for sub in subsets():
        for order in itertools.permutations(sub):
            yield make_config('x', order)


class IdentityStability(unittest.TestCase):                                   # P1
    def test_ref_is_the_same_under_every_selection(self):
        n = 0
        for cfg in configs():
            for ref in ALL_REFS:
                for dt in ('name', 'population'):
                    r = query(CAT, cfg, ref, dt)
                    n += 1
                    self.assertEqual(r.ref, ref)
                    self.assertIn(r.status, (OK, DETAIL_UNAVAILABLE))
        self.assertEqual(n, 16 * len(ALL_REFS) * 2)      # 1+3+6+6 ordered subsets of three sources

    def test_empty_configuration_still_knows_every_entity(self):
        empty = make_config('none', ())
        for ref in ALL_REFS:
            self.assertEqual(query(CAT, empty, ref, 'name').status, DETAIL_UNAVAILABLE)

    def test_all_references_live_in_synthetic_range(self):
        self.assertTrue(all(r & SYNTHETIC for r in list(CAT.kinds) + list(CAT.retired)))


class QualifiedQueries(unittest.TestCase):                                    # P2
    def test_exact_source_and_revision(self):
        cfg = make_config('wi', (W, WS, C))
        r = query(CAT, cfg, MADISON_CITY, 'population', (WS.id, 7))
        self.assertEqual((r.status, r.value, r.source, r.revision), (OK, 270, WS.id, 7))

    def test_never_falls_back_when_source_removed(self):
        cfg = make_config('world', (W,), pop=[WS.id, W.id])   # priorities still name wisc
        pinned = query(CAT, cfg, MADISON_CITY, 'population', (WS.id, 7))
        ctx = query(CAT, cfg, MADISON_CITY, 'population')
        self.assertEqual(pinned.status, SOURCE_MISSING)
        self.assertIsNone(pinned.value)
        self.assertEqual((ctx.status, ctx.value, ctx.source), (OK, 100, W.id))   # context may fall back; pin may not

    def test_revision_mismatch_is_not_a_silent_upgrade(self):
        cfg = make_config('wi', (WS,))
        r = query(CAT, cfg, MADISON_CITY, 'population', (WS.id, 6))
        self.assertEqual((r.status, r.value), (REVISION_MISSING, None))

    def test_pinned_source_without_detail_reports_detail_unavailable(self):
        cfg = make_config('both', (W, WS))
        r = query(CAT, cfg, MONACO, 'population', (W.id, 3))                  # known, record has no population
        self.assertEqual((r.status, r.value), (DETAIL_UNAVAILABLE, None))

    def test_pin_ignores_priority_order(self):
        for pop in ([C.id, WS.id], [WS.id, C.id]):
            cfg = make_config('wi', (WS, C), pop=pop)
            self.assertEqual(query(CAT, cfg, WI, 'population', (C.id, 2)).value, 5900)


class ContextualQueries(unittest.TestCase):                                   # P3
    def test_reports_chosen_skipped_and_conflict(self):
        cfg = make_config('wi', (W, WS, C), pop=[C.id, WS.id, W.id])
        r = query(CAT, cfg, MADISON_CITY, 'population')
        self.assertEqual((r.value, r.source, r.revision), (280, C.id, 2))
        self.assertEqual(r.provenance, ((C.id, 2, 'chosen'), (WS.id, 7, 'shadowed'), (W.id, 3, 'shadowed')))
        self.assertEqual(r.conflicts, ((WS.id, 7, 270), (W.id, 3, 100)))

    def test_priority_change_changes_provenance_not_identity(self):
        a = query(CAT, make_config('wi', (WS, C), pop=[WS.id, C.id]), MADISON_CITY, 'population')
        b = query(CAT, make_config('wi', (WS, C), pop=[C.id, WS.id]), MADISON_CITY, 'population')
        self.assertEqual((a.ref, a.value, b.value), (b.ref, 270, 280))
        self.assertNotEqual(a.source, b.source)

    def test_missing_higher_priority_source_is_listed_not_hidden(self):
        cfg = make_config('world', (W,), pop=[WS.id, W.id])
        r = query(CAT, cfg, MADISON_CITY, 'population')
        self.assertEqual(r.provenance[0], (WS.id, None, 'source-missing'))
        self.assertEqual(r.source, W.id)

    def test_agreeing_lower_source_is_not_a_conflict(self):
        both = type(C)(C.id, 2, C.records)
        cfg = make_config('x', (C, both))
        self.assertEqual(query(CAT, cfg, MADISON_CITY, 'population').conflicts, ())

    def test_unrelated_source_does_not_reorder(self):
        base = query(CAT, make_config('x', (WS, C), pop=[WS.id, C.id]), MADISON_CITY, 'population')
        more = query(CAT, make_config('x', (W, WS, C), pop=[WS.id, C.id]), MADISON_CITY, 'population')
        self.assertEqual((base.source, base.value), (more.source, more.value))


class DistinctStatuses(unittest.TestCase):                                    # P4
    def test_five_situations_five_statuses(self):
        world = make_config('world', (W,))
        wi_gone = make_config('wi', (WS,), pop=[WS.id, 'gone.example/x'])
        got = {
            ENTITY_UNKNOWN: query(CAT, world, UNKNOWN, 'name').status,
            DETAIL_UNAVAILABLE: query(CAT, world, WI, 'name').status,          # known, no installed record
            SOURCE_MISSING: query(CAT, wi_gone, WI, 'name', ('gone.example/x', 1)).status,
            AMBIGUOUS: lookup_name(CAT, world, 'Madison').status,
            RETIRED: query(CAT, world, OLD_VILLAGE, 'name').status,
            SPLIT: query(CAT, world, OLD_COUNTY, 'name').status}
        for want, have in got.items():
            self.assertEqual(want, have)
        self.assertEqual(len(set(got.values())), 6)

    def test_absent_optional_source_is_not_entity_unknown(self):
        self.assertNotEqual(query(CAT, make_config('none', ()), WI, 'name').status, ENTITY_UNKNOWN)

    def test_retired_and_split_do_not_consult_sources(self):
        r = query(CAT, make_config('none', ()), OLD_VILLAGE, 'name')
        self.assertEqual((r.status, r.successors), (RETIRED, (MADISON_CITY,)))


class NoInferredIdentity(unittest.TestCase):                                  # P5
    def test_proposed_concordance_contributes_nothing(self):
        cfg = make_config('world', (W,))
        r = query(CAT, cfg, MADISON_CITY, 'population')
        self.assertEqual((r.value, r.conflicts), (100, ()))     # lookalike (5) same name+coordinates ignored
        self.assertEqual(query(CAT, cfg, GHOST, 'population').status, DETAIL_UNAVAILABLE)
        self.assertEqual(lookup_name(CAT, cfg, 'Lake Ghost').status, ENTITY_UNKNOWN)

    def test_unlinked_matching_record_is_not_found_by_name_join(self):
        cfg = make_config('world', (W,))
        self.assertEqual(lookup_name(CAT, cfg, 'Madison').successors, (MADISON_CITY, MADISON_IN))

    def test_ambiguous_name_never_returns_a_first_match(self):
        r = lookup_name(CAT, make_config('world', (W,)), 'Madison')
        self.assertEqual((r.status, r.ref), (AMBIGUOUS, 0))

    def test_town_and_city_stay_separate_in_wisconsin_edition(self):
        r = lookup_name(CAT, make_config('wi', (WS,)), 'Madison')
        self.assertEqual((r.status, r.successors), (AMBIGUOUS, (MADISON_CITY, MADISON_TOWN)))

    def test_name_unique_in_one_edition_is_ambiguous_in_the_union(self):
        self.assertEqual(lookup_name(CAT, make_config('w', (W,)), 'Springfield').ref, SPRINGFIELD_W)
        self.assertEqual(lookup_name(CAT, make_config('wi', (WS,)), 'Springfield').ref, SPRINGFIELD_WI)
        self.assertEqual(lookup_name(CAT, make_config('both', (W, WS)), 'Springfield').status, AMBIGUOUS)


class SemanticRecording(unittest.TestCase):                                   # P6, P7
    def test_ref_text_round_trips_every_synthetic_id(self):
        for ref in list(CAT.kinds) + [0, 1, 0xFFFFFFFF]:
            self.assertEqual(parse_text(format_ref('X', ref))[1], ref)
            self.assertEqual(decode_ref(encode_ref(ref)), ref)
        self.assertEqual(len(encode_ref(0xFFFFFFFF)), 7)

    def test_hint_is_not_authoritative(self):
        cfg = make_config('wi', (WS, C))
        r = resolve_text(CAT, cfg, format_ref('Springfield', MADISON_CITY))
        self.assertEqual(r.ref, MADISON_CITY)
        self.assertIn('hint-mismatch', r.diagnostic)

    def test_malformed_suffix_is_an_error_not_a_name(self):
        for text in ('Madison~', 'Madison~abc', 'Madison~ILOU123', 'a~b'):
            r = resolve_text(CAT, make_config('wi', (WS,)), text)
            self.assertEqual(r.status, ENTITY_UNKNOWN, text)
            self.assertEqual(r.diagnostic, 'malformed-suffix')

    def test_suffix_beyond_32_bits_is_rejected(self):
        self.assertEqual(parse_text('X~ZZZZZZZ')[2], 'suffix-out-of-range')

    def test_replay_follows_ref_when_names_change(self):
        cfg = make_config('wi', (WS, C))
        rec = record_command(CAT, cfg, 'show', MADISON_CITY, 'population')
        renamed = type(WS)(WS.id, 8, tuple((k, type(v)('Madison Heights', v.lat, v.lon,
                                                      tuple(('name', 'Madison Heights') if t == 'name' else (t, x)
                                                            for t, x in v.claims)))
                                           if k == 'mad' else (k, v) for k, v in WS.records))
        after = make_config('wi', (renamed, C))
        res = replay(CAT, after, rec)
        self.assertEqual((res.ref, res.status), (MADISON_CITY, OK))
        self.assertIn('snapshot-drift', res.diagnostic)
        self.assertEqual(resolve_text(CAT, after, rec['readable']).ref, MADISON_CITY)
        self.assertIn('hint-mismatch', resolve_text(CAT, after, rec['readable']).diagnostic)

    def test_hint_only_form_is_ambiguous_where_ids_are_not(self):
        cfg = make_config('wi', (WS,))
        self.assertEqual(resolve_text(CAT, cfg, 'Madison').status, AMBIGUOUS)
        self.assertEqual(resolve_text(CAT, cfg, format_ref('Madison', MADISON_TOWN)).ref, MADISON_TOWN)

    def test_record_has_readable_text_and_authoritative_ref(self):
        rec = record_command(CAT, make_config('wi', (WS,)), 'show', MADISON_CITY, 'name')
        self.assertTrue(rec['readable'].startswith('Madison~'))
        self.assertEqual(parse_text(rec['readable'])[1], rec['ref'])

    def test_snapshot_id_is_order_independent_and_change_sensitive(self):
        ids = {make_config('x', (W, WS, C)).snapshot_id,
               make_config('x', (C, W, WS), names=[W.id, WS.id, C.id], pop=[W.id, WS.id, C.id]).snapshot_id}
        self.assertEqual(len(ids), 1)
        base = make_config('x', (W, WS))
        changed = {
            make_config('x', (W,), pop=[W.id, WS.id], names=[W.id, WS.id]).snapshot_id,        # removal
            make_config('x', (W, WS), pop=[WS.id, W.id]).snapshot_id,                           # priority
            make_config('y', (W, WS)).snapshot_id,                                              # edition
            make_config('x', (W, type(WS)(WS.id, 8, WS.records))).snapshot_id}                  # revision
        self.assertNotIn(base.snapshot_id, changed)
        self.assertEqual(len(changed), 4)

    def test_pinned_records_replay_identically_across_drift(self):
        a = make_config('wi', (WS, C))
        rec = record_command(CAT, a, 'show', MADISON_CITY, 'population', (C.id, 2))
        b = make_config('wi', (C, WS), pop=[C.id, WS.id])
        c = make_config('wi', (C,))
        self.assertEqual(replay(CAT, a, rec), replay(CAT, b, rec))
        self.assertEqual(replay(CAT, a, rec), replay(CAT, c, rec))

    def test_contextual_record_drift_is_reported_or_refused(self):
        a = make_config('wi', (WS, C), pop=[WS.id, C.id])
        rec = record_command(CAT, a, 'show', MADISON_CITY, 'population')
        b = make_config('wi', (WS, C), pop=[C.id, WS.id])
        self.assertEqual(replay(CAT, a, rec).value, 270)
        loose, strict = replay(CAT, b, rec), replay(CAT, b, rec, strict=True)
        self.assertEqual((loose.value, loose.ref), (280, MADISON_CITY))
        self.assertIn('snapshot-drift', loose.diagnostic)
        self.assertEqual((strict.status, strict.value), (SNAPSHOT_DRIFT, None))


class PendingResolution(unittest.TestCase):                                   # P8
    def setUp(self):
        self.world = make_config('world', (W,))
        self.wisc = make_config('wisc', (WS, C))

    def test_stale_completion_cannot_overwrite_and_follow_up_keeps_the_ref(self):
        m, (eff,) = update(Model(self.world), Navigate(SPRINGFIELD_W))
        stale = run_effect(CAT, self.world, eff)                                  # handler finished on old config
        m, (eff2,) = update(m, ConfigChanged(self.wisc))                          # user switches edition first
        before = m
        m, fx = update(m, stale)
        self.assertEqual((m.selected, m.committed, m.history, fx), (0, 0, (), []))
        self.assertEqual(m.pending, before.pending)
        self.assertEqual(m.log[-1], ('dropped', SPRINGFIELD_W))
        self.assertEqual(eff2.ref, SPRINGFIELD_W)                                 # not re-resolved by name to SPRINGFIELD_WI
        m, _ = update(m, run_effect(CAT, self.wisc, eff2))
        self.assertEqual(m.committed, SPRINGFIELD_W)                              # known entity, detail unavailable here
        self.assertEqual(query(CAT, self.wisc, SPRINGFIELD_W, 'name').status, DETAIL_UNAVAILABLE)

    def test_name_is_never_reresolved_after_a_switch(self):
        for text, expect in (('Springfield', SPRINGFIELD_W),):
            ref = resolve_text(CAT, self.world, text).ref
            self.assertEqual(ref, expect)
            m, (eff,) = update(Model(self.world), Navigate(ref))
            m, (eff2,) = update(m, ConfigChanged(self.wisc))
            self.assertEqual(lookup_name(CAT, self.wisc, text).ref, SPRINGFIELD_WI)   # what rebinding would pick
            self.assertEqual(eff2.ref, ref)

    def test_forged_or_mismatched_completions_are_dropped(self):
        m, (eff,) = update(Model(self.world), Navigate(SPRINGFIELD_W))
        good = run_effect(CAT, self.world, eff)
        for bad in (Resolved(good.token + 1, good.generation, good.ref, good.purpose, good.result),
                    Resolved(good.token, good.generation + 1, good.ref, good.purpose, good.result),
                    Resolved(good.token, good.generation, MADISON_CITY, good.purpose,
                             query(CAT, self.world, MADISON_CITY, 'name')),
                    Resolved(good.token, good.generation, good.ref, 'select', good.result),
                    Resolved(good.token, good.generation, good.ref, good.purpose,
                             query(CAT, self.world, MADISON_CITY, 'name'))):          # result for another ref
            m2, _ = update(m, bad)
            self.assertEqual((m2.committed, m2.selected, m2.pending), (0, 0, eff), bad)
        m, _ = update(m, good)
        self.assertEqual(m.committed, SPRINGFIELD_W)

    def test_select_does_not_create_history(self):
        m, (eff,) = update(Model(self.world), Navigate(SPRINGFIELD_W, 'select'))
        m, _ = update(m, run_effect(CAT, self.world, eff))
        self.assertEqual((m.selected, m.committed, m.history), (SPRINGFIELD_W, 0, ()))

    def test_second_navigation_supersedes_first(self):
        m, (e1,) = update(Model(self.world), Navigate(SPRINGFIELD_W))
        m, (e2,) = update(m, Navigate(MADISON_IN))
        m, _ = update(m, run_effect(CAT, self.world, e1))
        self.assertEqual(m.committed, 0)
        m, _ = update(m, run_effect(CAT, self.world, e2))
        self.assertEqual(m.committed, MADISON_IN)

    def test_failed_resolution_preserves_view_and_history(self):
        m, (e,) = update(Model(self.world), Navigate(SPRINGFIELD_W))
        m, _ = update(m, run_effect(CAT, self.world, e))
        m, (e,) = update(m, Navigate(UNKNOWN))
        m, _ = update(m, run_effect(CAT, self.world, e))
        self.assertEqual((m.committed, m.history, m.log[-1]), (SPRINGFIELD_W, (), (ENTITY_UNKNOWN, UNKNOWN)))

    def test_every_interleaving_ends_on_the_requested_ref(self):
        order = [self.world, self.wisc, make_config('none', ())]
        for pattern in itertools.product(range(3), repeat=3):
            m, (eff,) = update(Model(order[0]), Navigate(MADISON_CITY))
            handled = [run_effect(CAT, order[0], eff)]
            for i in pattern:
                m, fx = update(m, ConfigChanged(order[i]))
                handled += [run_effect(CAT, order[i], f) for f in fx]
            for msg in handled:                                                  # deliver in recorded order
                m, _ = update(m, msg)
            for msg in reversed(handled):                                        # and a late duplicate replay
                m, _ = update(m, msg)
            self.assertIn(m.committed, (0, MADISON_CITY))
            self.assertTrue(m.committed == MADISON_CITY or m.pending is not None)


class OwnedResultsAndLeases(unittest.TestCase):                               # P9
    def test_results_are_plain_owned_values(self):
        for cfg in list(configs())[:20]:
            for ref in ALL_REFS + [OLD_COUNTY, UNKNOWN]:
                self.assertTrue(owned(query(CAT, cfg, ref, 'population')))

    def test_view_is_rejected_after_retirement_but_copied_result_survives(self):
        lib = Library()
        view = lib.view(WS)
        copied = view.read('mad')
        lib.retire()
        with self.assertRaises(StaleView):
            view.read('mad')
        self.assertEqual(copied.name, 'Madison')
        self.assertEqual(lib.view(WS).read('mad'), copied)

    def test_owned_rejects_a_live_view(self):
        self.assertFalse(owned((1, Library().view(WS))))


class RetirementTable(unittest.TestCase):                                     # P10
    ACTIVE = {1, 2, 3, 4}

    def test_chain_is_flattened_to_terminal_active(self):
        flat = flatten_retirements(self.ACTIVE, {10: ('replaced-by', 11), 11: ('replaced-by', 12), 12: ('replaced-by', 1)})
        self.assertEqual(flat, {10: ('retired', 1), 11: ('retired', 1), 12: ('retired', 1)})

    def test_split_is_never_collapsed(self):
        flat = flatten_retirements(self.ACTIVE, {10: ('split-into', (3, 2)), 9: ('replaced-by', 10)})
        self.assertEqual(flat[10], ('split', (2, 3)))
        self.assertEqual(flat[9], ('split', (2, 3)))

    def test_rejections(self):
        for raw in ({10: ('replaced-by', 11), 11: ('replaced-by', 10)},          # cycle
                    {10: ('replaced-by', 99)},                                    # dangling
                    {1: ('replaced-by', 2)},                                      # id reuse
                    {10: ('split-into', (2, 99))},                                # dangling split member
                    {10: ('split-into', (11, 2)), 11: ('split-into', (3, 4))}):    # nested split
            with self.assertRaises(ValueError, msg=raw):
                flatten_retirements(self.ACTIVE, raw)

    def test_runtime_lookup_is_one_step(self):
        self.assertEqual(CAT.retired[OLD_VILLAGE], ('retired', MADISON_CITY))
        self.assertTrue(all(v[0] in ('retired', 'split') and
                            (v[1] in CAT.kinds if v[0] == 'retired' else set(v[1]) <= set(CAT.kinds))
                            for v in CAT.retired.values()))
        self.assertEqual(query(CAT, make_config('x', ()), OLD_COUNTY, 'name').successors, (COUNTY_A, COUNTY_B))


class RemovedEdition(unittest.TestCase):
    def test_held_reference_degrades_to_detail_unavailable_not_unknown(self):
        held = [WI, MADISON_CITY, MADISON_TOWN, SPRINGFIELD_WI]
        installed = make_config('wi', (W, WS, C))
        removed = make_config('world', (W,))
        for ref in held:
            self.assertNotEqual(query(CAT, installed, ref, 'name').status, ENTITY_UNKNOWN)
            self.assertNotEqual(query(CAT, removed, ref, 'name').status, ENTITY_UNKNOWN)
        self.assertEqual(query(CAT, removed, WI, 'name').status, DETAIL_UNAVAILABLE)
        self.assertEqual(query(CAT, removed, GHOST, 'name').status, DETAIL_UNAVAILABLE)

    def test_pinned_wisconsin_query_after_removal_is_source_missing(self):
        self.assertEqual(query(CAT, make_config('world', (W,)), WI, 'name', (WS.id, 7)).status, SOURCE_MISSING)


if __name__ == '__main__':
    unittest.main()
