#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""Synthetic model of edition-selection semantics (EXP-EDSEL-01). Research only.

Models *contract semantics*, not C++ layout: canonical references, approved concordances,
contextual versus source-qualified queries, source-configuration snapshots, semantic command
records and generation-safe asynchronous resolution. All data are invented. No file is read.
"""
from dataclasses import dataclass, replace
import hashlib
import json
import re

SYNTHETIC = 0x80000000          # reserved synthetic range (entity-navigation-contract.md)

# Result statuses. Each names a distinct situation; none is derived from another.
OK, DETAIL_UNAVAILABLE, ENTITY_UNKNOWN = 'ok', 'detail-unavailable', 'entity-unknown'
SOURCE_MISSING, REVISION_MISSING = 'source-missing', 'revision-missing'
AMBIGUOUS, RETIRED, SPLIT = 'ambiguous', 'retired', 'split'
SNAPSHOT_DRIFT = 'snapshot-drift'
PRECEDENCE_UNRESOLVED = 'precedence-unresolved'    # overlapping claims disagree; no explicit priority covers them
SOURCE_CONFLICT = 'source-conflict'                # one source contradicts itself for this entity and datatype


@dataclass(frozen=True)
class Record:
    name: str
    lat: float
    lon: float
    claims: tuple = ()           # ((datatype, value), ...); absent type means no detail

    def claim(self, datatype):
        return dict(self.claims).get(datatype)


@dataclass(frozen=True)
class Source:
    id: str                      # publisher-qualified, e.g. 'wisc.example/places'
    revision: int
    records: tuple               # ((local_id, Record), ...)
    supersedes: tuple = ()       # declared publication metadata (policy B only); never consulted by query()

    @property
    def digest(self):
        blob = json.dumps([self.id, self.revision, sorted(self.supersedes),
                           sorted([k, v.name, v.lat, v.lon, list(v.claims)] for k, v in self.records)],
                          sort_keys=True)
        return hashlib.sha256(blob.encode()).hexdigest()[:16]

    def get(self, local_id):
        return dict(self.records).get(local_id)


@dataclass(frozen=True)
class Catalog:
    """Globe-owned and embedded: always present, independent of installed editions."""
    kinds: dict                  # active ref -> kind
    concordance: dict            # (source id, local id) -> (ref, 'approved' | 'proposed')
    retired: dict                # flat builder table: ref -> ('retired', succ) | ('split', (a, b))

    def approved_refs(self, source_id, local_id):
        hit = self.concordance.get((source_id, local_id))
        return hit[0] if hit and hit[1] == 'approved' else None


@dataclass(frozen=True)
class Config:
    """Source-selection context: what the user switches. Never part of an entity's identity."""
    edition: str
    installed: tuple             # (Source, ...) as a set: order carries no meaning
    priority: tuple              # ((datatype, (source id, ...)), ...) explicit user choice only

    def __post_init__(self):
        seen = {}
        for src in self.installed:
            if src.id in seen and seen[src.id].digest != src.digest:
                raise ValueError(f'duplicate source identity {src.id!r} with different content or revision')
            seen[src.id] = src
        object.__setattr__(self, 'installed', tuple(sorted(seen.values(), key=lambda x: x.id)))
        for dt, order in self.priority:
            if len(set(order)) != len(order):
                raise ValueError(f'priority for {dt!r} lists a source twice')

    def source(self, source_id):
        return next((s for s in self.installed if s.id == source_id), None)

    def order(self, datatype):
        return dict(self.priority).get(datatype, ())

    @property
    def snapshot_id(self):
        blob = json.dumps({'edition': self.edition,
                           'sources': sorted([s.id, s.revision, s.digest] for s in self.installed),
                           'priority': sorted([t, list(o)] for t, o in self.priority)},
                          sort_keys=True)
        return hashlib.sha256(blob.encode()).hexdigest()[:16]


@dataclass(frozen=True)
class Result:
    """Owned value. Only str/int/float/None/tuples of those: no view, pointer or handle."""
    status: str
    ref: int
    value: object = None
    source: str = None
    revision: int = None
    provenance: tuple = ()       # ((source id, revision or None, outcome), ...) in priority order
    conflicts: tuple = ()        # ((source id, revision, value), ...) disagreeing lower sources
    successors: tuple = ()       # terminal refs for retired / split, candidates for ambiguous
    diagnostic: str = ''


# ---------------------------------------------------------------- resolution

def classify(cat, ref):
    """Identity only. Never consults sources, so it cannot change with a configuration."""
    if ref in cat.kinds:
        return None
    hit = cat.retired.get(ref)
    if hit is None:
        return Result(ENTITY_UNKNOWN, ref, diagnostic='not in the canonical catalog')
    if hit[0] == 'retired':
        return Result(RETIRED, ref, successors=(hit[1],))
    return Result(SPLIT, ref, successors=tuple(hit[1]), diagnostic='no successor chosen')


def _claims(cat, source, ref, datatype):
    """Every approved local record's claim for ref: [(local id, value)], sorted so record order is irrelevant."""
    out = [(k, r.claim(datatype)) for k, r in source.records
           if cat.approved_refs(source.id, k) == ref and r.claim(datatype) is not None]
    return sorted(out, key=lambda kv: (kv[0], repr(kv[1])))


def _source_value(cat, source, ref, datatype):
    """-> ('none', None) | ('value', v) | ('conflict', ((local id, v), ...)). Equal duplicates are one value."""
    claims = _claims(cat, source, ref, datatype)
    values = {repr(v): v for _, v in claims}
    if not claims:
        return 'none', None
    if len(values) == 1:
        return 'value', claims[0][1]
    return 'conflict', tuple(claims)


def validate_source(cat, source):
    """Builder-side check (policy A for Finding B). Raises ValueError listing every contradiction."""
    bad = []
    for ref in sorted({cat.approved_refs(source.id, k) for k, _ in source.records} - {None}):
        for dt in sorted({t for k, r in source.records if cat.approved_refs(source.id, k) == ref
                          for t, _ in r.claims}):
            kind, detail = _source_value(cat, source, ref, dt)
            if kind == 'conflict':
                bad.append((ref, dt, detail))
    if bad:
        raise ValueError(f'{source.id} contradicts itself: {bad}')


def _candidates(items):
    """Deterministic, order-free candidate list: (source id, revision, value)."""
    return tuple(sorted(items, key=lambda c: (c[0], c[1], repr(c[2]))))


def query(cat, cfg, ref, datatype, pin=None):
    """Contextual when pin is None, else (source id, revision) qualified; qualified never falls back."""
    bad = classify(cat, ref)
    if bad:
        return bad
    if pin is not None:
        sid, rev = pin
        src = cfg.source(sid)
        if src is None:
            return Result(SOURCE_MISSING, ref, source=sid, revision=rev,
                          diagnostic='qualifying source not installed')
        if src.revision != rev:
            return Result(REVISION_MISSING, ref, source=sid, revision=rev,
                          diagnostic=f'installed revision is {src.revision}')
        kind, val = _source_value(cat, src, ref, datatype)
        if kind == 'value':
            return Result(OK, ref, val, sid, rev, ((sid, rev, 'chosen'),))
        if kind == 'conflict':
            return Result(SOURCE_CONFLICT, ref, source=sid, revision=rev,
                          provenance=((sid, rev, 'conflict'),),
                          conflicts=_candidates((sid, rev, v) for _, v in val),
                          diagnostic='source contradicts itself: ' + ','.join(k for k, _ in val))
        return Result(DETAIL_UNAVAILABLE, ref, source=sid, revision=rev,
                      provenance=((sid, rev, 'no-claim'),), diagnostic='pinned source lacks detail')
    order = cfg.order(datatype)
    claimants, bad_sources = [], []                    # claimants: (sid, rev, value)
    for src in cfg.installed:                          # cfg.installed is id-sorted: no install-order leak
        kind, val = _source_value(cat, src, ref, datatype)
        if kind == 'conflict':
            bad_sources.append((src.id, src.revision, val))
        elif kind == 'value':
            claimants.append((src.id, src.revision, val))

    def trail(first=None):
        """Explicit-priority order first (missing and claimless sources included), then unranked claimants."""
        out, have = [], {c[0]: c for c in claimants}
        for sid in order:
            src = cfg.source(sid)
            if src is None:
                out.append((sid, None, 'source-missing'))
            elif sid in have:
                out.append((sid, src.revision, 'chosen' if have[sid] == first else 'shadowed'))
            else:
                out.append((sid, src.revision, 'no-claim'))
        out += sorted((c[0], c[1], 'unranked') for c in claimants if c[0] not in order)
        return tuple(out)

    if bad_sources:                                    # never choose around a self-contradicting source
        cands = _candidates((sid, rev, v) for sid, rev, vals in bad_sources for _, v in vals)
        return Result(SOURCE_CONFLICT, ref, provenance=tuple(sorted((a, b, 'conflict') for a, b, _ in bad_sources)),
                      conflicts=cands, diagnostic='a source contradicts itself')
    if not claimants:
        return Result(DETAIL_UNAVAILABLE, ref, provenance=trail(),
                      diagnostic='entity known; no installed source has this detail')
    distinct = {repr(v) for _, _, v in claimants}
    ranked = [c for sid in order for c in claimants if c[0] == sid]
    if len(distinct) > 1 and len(ranked) != len(claimants):          # a disagreeing claimant has no rank
        return Result(PRECEDENCE_UNRESOLVED, ref, conflicts=_candidates(claimants), provenance=trail(),
                      diagnostic='claims disagree and no explicit priority covers every claimant')
    if len(claimants) == 1:                                          # lone claimant: it is the source, no rank needed
        sid, rev, val = claimants[0]
        return Result(OK, ref, val, sid, rev, trail(claimants[0]))
    if len(distinct) == 1 and not ranked:                            # several agree: no precedence needed
        return Result(OK, ref, claimants[0][2], None, None,
                      tuple(sorted((c[0], c[1], 'agreeing') for c in claimants)))
    first = ranked[0]
    conflicts = tuple(c for c in ranked[1:] if repr(c[2]) != repr(first[2]))
    return Result(OK, ref, first[2], first[0], first[1], trail(first), conflicts)


def lookup_name(cat, cfg, name):
    """Name -> reference through approved concordances only. Several candidates are ambiguous."""
    refs = set()
    for src in cfg.installed:
        for k, r in src.records:
            ref = cat.approved_refs(src.id, k)
            if ref is not None and r.name == name:
                refs.add(ref)
    if not refs:
        return Result(ENTITY_UNKNOWN, 0, diagnostic=f'no installed source names {name!r}')
    if len(refs) > 1:
        return Result(AMBIGUOUS, 0, successors=tuple(sorted(refs)),
                      diagnostic='several entities share this name')
    return Result(OK, next(iter(refs)))


def display_name(cat, cfg, ref):
    r = query(cat, cfg, ref, 'name')
    return r.value if r.status == OK else ''


# ------------------------------------------------- textual references (hint~ID)

_B32 = '0123456789ABCDEFGHJKMNPQRSTVWXYZ'          # Crockford: no I L O U
_SUFFIX = re.compile(r'~([0-9A-HJKMNP-TV-Z]{7})$')


def encode_ref(ref):
    return ''.join(_B32[(ref >> s) & 31] for s in range(30, -1, -5))     # 7 chars carry 32 bits


def decode_ref(text):
    n = 0
    for ch in text:
        n = (n << 5) | _B32.index(ch)
    if n > 0xFFFFFFFF:
        raise ValueError('out of range')
    return n


def format_ref(name, ref):
    return f'{name}~{encode_ref(ref)}'


def parse_text(text):
    """-> (hint, ref or None, error or ''). A trailing '~word' that is not a valid suffix is an error."""
    m = _SUFFIX.search(text)
    if m:
        try:
            return text[:m.start()], decode_ref(m.group(1)), ''
        except ValueError:
            return text[:m.start()], None, 'suffix-out-of-range'
    if re.search(r'~[^~]*$', text):
        return text, None, 'malformed-suffix'
    return text, None, ''


def resolve_text(cat, cfg, text):
    hint, ref, err = parse_text(text)
    if err:
        return Result(ENTITY_UNKNOWN, 0, diagnostic=err)
    if ref is None:
        return lookup_name(cat, cfg, hint)
    bad = classify(cat, ref)
    out = bad or Result(OK, ref)
    shown = display_name(cat, cfg, ref)
    if hint and shown and hint != shown:
        out = replace(out, diagnostic=(out.diagnostic + ' hint-mismatch').strip())
    return out


# -------------------------------------------------------- semantic recording

def record_command(cat, cfg, verb, ref, datatype=None, pin=None):
    """Readable description plus authoritative reference; snapshot stored only for contextual queries."""
    r = {'verb': verb, 'ref': ref, 'readable': format_ref(display_name(cat, cfg, ref), ref),
         'datatype': datatype, 'pin': pin, 'snapshot': None if pin else cfg.snapshot_id}
    return r


def replay(cat, cfg, rec, strict=False):
    if rec['pin'] is None and rec['snapshot'] != cfg.snapshot_id:
        if strict:
            return Result(SNAPSHOT_DRIFT, rec['ref'], diagnostic='contextual record replayed on another configuration')
        res = query(cat, cfg, rec['ref'], rec['datatype'])
        return replace(res, diagnostic=(res.diagnostic + ' snapshot-drift').strip())
    return query(cat, cfg, rec['ref'], rec['datatype'], rec['pin'])


# --------------------------------------------------- builder: retirement table

def flatten_retirements(active, raw):
    """raw: ref -> ('replaced-by', ref) | ('split-into', (refs...)). Returns the flat published table."""
    flat = {}
    for ref in raw:
        if ref in active:
            raise ValueError(f'retired id {ref:#x} is also active (reuse)')
    for ref in raw:
        seen, cur = [ref], ref
        while cur in raw and raw[cur][0] == 'replaced-by':
            cur = raw[cur][1]
            if cur in seen:
                raise ValueError('cycle')
            seen.append(cur)
        if cur in raw:                                   # chain ends in a split
            succ = tuple(sorted(raw[cur][1]))
            for s in succ:
                if s not in active and s not in raw:
                    raise ValueError('dangling successor')
            flat[ref] = ('split', tuple(_terminal(active, raw, s) for s in succ))
        elif cur in active:
            flat[ref] = ('retired', cur)
        else:
            raise ValueError('dangling successor')
    return flat


def _terminal(active, raw, ref):
    seen = set()
    while ref in raw and raw[ref][0] == 'replaced-by':
        if ref in seen:
            raise ValueError('cycle')
        seen.add(ref)
        ref = raw[ref][1]
    if ref in raw:
        raise ValueError('split successor is itself split')
    return ref


# ------------------------------------------------- generation-leased pack view

class StaleView(Exception):
    pass


class Library:
    """Stands in for the evictable resource owner. Views die with their generation."""
    def __init__(self):
        self.generation = 0

    def view(self, source):
        return _View(self, self.generation, source)

    def retire(self):
        self.generation += 1


class _View:
    def __init__(self, lib, gen, source):
        self._lib, self._gen, self._source = lib, gen, source

    def read(self, local_id):
        if self._gen != self._lib.generation:
            raise StaleView('view outlived its generation')
        return self._source.get(local_id)


# -------------------------------------- Elm-style model: asynchronous resolution

@dataclass(frozen=True)
class Navigate:
    ref: int
    purpose: str = 'navigate'    # or 'select'


@dataclass(frozen=True)
class Resolved:
    token: int
    generation: int
    ref: int
    purpose: str
    result: Result


@dataclass(frozen=True)
class ConfigChanged:
    cfg: Config


@dataclass(frozen=True)
class ResolveEffect:
    token: int
    generation: int
    ref: int
    purpose: str


@dataclass(frozen=True)
class Model:
    cfg: Config
    generation: int = 0
    next_token: int = 1
    pending: object = None       # ResolveEffect | None  (one in flight, per ADR 0005)
    selected: int = 0
    committed: int = 0           # reference of the last committed navigation
    history: tuple = ()
    log: tuple = ()              # bounded diagnostics: (event, detail)


def update(model, msg):
    if isinstance(msg, (Navigate,)):
        eff = ResolveEffect(model.next_token, model.generation, msg.ref, msg.purpose)
        return replace(model, next_token=model.next_token + 1, pending=eff), [eff]
    if isinstance(msg, ConfigChanged):
        gen = model.generation + 1
        m = replace(model, cfg=msg.cfg, generation=gen)
        if model.pending is None:
            return m, []
        p = model.pending                          # same reference and purpose, never the old name
        eff = ResolveEffect(m.next_token, gen, p.ref, p.purpose)
        return replace(m, next_token=m.next_token + 1, pending=eff,
                       log=m.log + (('reissued', p.ref),)), [eff]
    p = model.pending
    why = (None if p and msg.token == p.token and msg.generation == model.generation
           and msg.ref == p.ref and msg.purpose == p.purpose and msg.result.ref == p.ref else 'stale')
    if why:
        return replace(model, log=model.log + (('dropped', msg.ref),)), []
    m = replace(model, pending=None)
    if msg.result.status not in (OK, DETAIL_UNAVAILABLE):
        return replace(m, log=m.log + ((msg.result.status, msg.ref),)), []
    if msg.purpose == 'select':
        return replace(m, selected=msg.ref), []
    hist = (m.history + (m.committed,)) if m.committed else m.history
    return replace(m, selected=msg.ref, committed=msg.ref, history=hist), []


def run_effect(cat, cfg, eff, datatype='name'):
    """The effect handler. Stamps the effect's own generation: it cannot see the model's."""
    return Resolved(eff.token, eff.generation, eff.ref, eff.purpose, query(cat, cfg, eff.ref, datatype))


def owned(value):
    """True if a result holds only plain values (no live handle)."""
    if isinstance(value, (str, int, float, type(None))):
        return True
    if isinstance(value, tuple):
        return all(owned(v) for v in value)
    if isinstance(value, Result):
        return all(owned(getattr(value, f)) for f in value.__dataclass_fields__)
    return False


# ------------------------------------------------------------------ fixture

WI, MADISON_CITY, MADISON_TOWN, MADISON_IN, MONACO, SPRINGFIELD_W, SPRINGFIELD_WI, GHOST = (
    SYNTHETIC + i for i in (1, 2, 3, 4, 5, 6, 7, 8))
OLD_VILLAGE, OLD_COUNTY, COUNTY_A, COUNTY_B, UNKNOWN = (SYNTHETIC + i for i in (20, 21, 22, 23, 99))


def fixture():
    world = Source('world.example/places', 3, (
        ('lookalike', Record('Madison', 43.07, -89.40, (('name', 'Madison'), ('population', 5)))),
        ('lake-lookalike', Record('Lake Ghost', 44.0, -90.0, (('name', 'Lake Ghost'), ('population', 1)))),
        ('mad-wi', Record('Madison', 43.07, -89.40, (('name', 'Madison'), ('population', 100)))),
        ('mad-in', Record('Madison', 38.73, -85.38, (('name', 'Madison'), ('population', 12)))),
        ('mc', Record('Monaco', 43.73, 7.42, (('name', 'Monaco'),))),      # name only: no population
        ('spr', Record('Springfield', 39.80, -89.65, (('name', 'Springfield'),)))))
    wisc = Source('wisc.example/places', 7, (
        ('wi', Record('Wisconsin', 44.5, -89.5, (('name', 'Wisconsin'), ('population', 5800)))),
        ('mad', Record('Madison', 43.07, -89.40, (('name', 'Madison'), ('population', 270)))),
        ('town', Record('Madison', 43.03, -89.42, (('name', 'Madison'), ('population', 6)))),
        ('spr', Record('Springfield', 44.1, -89.0, (('name', 'Springfield'),)))))
    census = Source('census.example/pop', 2, (
        ('wi', Record('Wisconsin', 44.5, -89.5, (('population', 5900),))),
        ('mad', Record('Madison', 43.07, -89.40, (('population', 280),)))))
    c = lambda sid, k, ref, st='approved': ((sid, k), (ref, st))
    cat = Catalog(
        kinds={WI: 'admin1', MADISON_CITY: 'city', MADISON_TOWN: 'town', MADISON_IN: 'city',
               MONACO: 'country', SPRINGFIELD_W: 'city', SPRINGFIELD_WI: 'city', GHOST: 'lake',
               COUNTY_A: 'county', COUNTY_B: 'county'},
        concordance=dict([
            c('world.example/places', 'mad-wi', MADISON_CITY), c('world.example/places', 'mad-in', MADISON_IN),
            c('world.example/places', 'mc', MONACO), c('world.example/places', 'spr', SPRINGFIELD_W),
            c('world.example/places', 'lookalike', MADISON_CITY, 'proposed'),    # same name and coordinates
            c('world.example/places', 'lake-lookalike', GHOST, 'proposed'),      # no approved record anywhere
            c('wisc.example/places', 'wi', WI), c('wisc.example/places', 'mad', MADISON_CITY),
            c('wisc.example/places', 'town', MADISON_TOWN), c('wisc.example/places', 'spr', SPRINGFIELD_WI),
            c('census.example/pop', 'wi', WI), c('census.example/pop', 'mad', MADISON_CITY)]),
        retired=flatten_retirements(
            {WI, MADISON_CITY, MADISON_TOWN, MADISON_IN, MONACO, SPRINGFIELD_W, SPRINGFIELD_WI, GHOST,
             COUNTY_A, COUNTY_B},
            {OLD_VILLAGE: ('replaced-by', MADISON_CITY), OLD_COUNTY: ('split-into', (COUNTY_A, COUNTY_B))}))
    return cat, {'world': world, 'wisc': wisc, 'census': census}


def make_config(edition, installed, pop=None, names=None):
    """Priorities exist only if the caller supplies them; installation order is never a priority."""
    pr = tuple((t, tuple(o)) for t, o in (('name', names), ('population', pop)) if o is not None)
    return Config(edition, tuple(installed), pr)


def ranked_config(edition, installed, pop=None, names=None):
    """The user explicitly ranks sources in the order given (an explicit choice, stored in the snapshot)."""
    ids = [s.id for s in installed]
    return make_config(edition, installed, pop=ids if pop is None else pop, names=ids if names is None else names)


def install_order_config(edition, installed):
    """DEFECTIVE BASELINE (review finding A): installation order silently becomes priority. Negative control only."""
    return ranked_config(edition, installed)


def alphabetical_order(installed):
    """Deterministic but arbitrary: control for 'sort the publishers and call it authoritative'."""
    return tuple(sorted(s.id for s in installed))


def derive_declared_order(installed):
    """Policy B: priority from declared `supersedes`, only if it is a strict total order; else None."""
    ids = {s.id for s in installed}
    sup = {s.id: set(s.supersedes) & ids for s in installed}
    for _ in ids:                                              # transitive closure
        for a in ids:
            sup[a] |= set().union(*[sup[b] for b in sup[a]]) if sup[a] else set()
    if any(a in sup[a] or any(a in sup[b] for b in sup[a]) for a in ids):
        return None                                            # cycle or mutual declaration
    order = sorted(ids, key=lambda a: -len(sup[a]))
    ok = all(order[j] in sup[order[i]] for i in range(len(order)) for j in range(i + 1, len(order)))
    return tuple(order) if ok else None
