#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2026 Jack Willis
"""EXP-FACTS-01. Synthetic research on a proposed facts contract, NOT Globe runtime."""
import copy
import hashlib
import json
import random

APPROVED_SOURCES = {"pinned-snapshot"}
DEFINITIONS = {"city proper", "metropolitan area", "census place"}


def claim(entity="E", value=120000, year=2021, definition="city proper",
          reviewed=True, source="pinned-snapshot", claim_id="c1"):
    return dict(id=claim_id, entity=entity, value=value, year=year,
                definition=definition, reviewed=reviewed, source=source)


def fixture(name, *, entity="E", identity_reviewed=True, preferred="c1",
            claims=None, expected="c1"):
    return dict(id=name, entity=entity, identity_reviewed=identity_reviewed,
                preferred=preferred, claims=[claim()] if claims is None else claims,
                expected=expected)


def cases():
    return [
        fixture("qualified"),
        fixture("missing-year", claims=[claim(year=None)], expected=None),
        fixture("missing-definition", claims=[claim(definition=None)], expected=None),
        fixture("unreviewed-identity", identity_reviewed=False, expected=None),
        fixture("wrong-subject", claims=[claim(entity="OTHER")], expected=None),
        fixture("unreviewed-assertion", claims=[claim(reviewed=False)], expected=None),
        fixture("unapproved-source", claims=[claim(source="untracked-url")], expected=None),
        fixture("curated-not-largest", preferred="curated",
                claims=[claim(value=300000, year=2018, claim_id="curated"),
                        claim(value=1300000, year=2022, definition="metropolitan area",
                              reviewed=False, claim_id="largest")],
                expected="curated"),
        fixture("ambiguous-preferred", claims=[
            claim(value=300000, year=2018),
            claim(value=320000, year=2021)], expected=None),
        fixture("no-curated-preference", preferred=None, expected=None),
    ]


def strict_select(case, *, require_year=True):
    """Candidate fail-closed policy. It does not exist in production code."""
    if case.get("identity_reviewed") is not True or not case.get("preferred"):
        return None
    matches = [c for c in case["claims"] if c["id"] == case["preferred"]]
    if len(matches) != 1:
        return None
    c = matches[0]
    if (c.get("entity") != case["entity"]
            or c.get("reviewed") is not True
            or c.get("source") not in APPROVED_SOURCES
            or type(c.get("value")) is not int or c["value"] <= 0
            or (require_year and (type(c.get("year")) is not int
                                  or not 1800 <= c["year"] <= 2026))
            or c.get("definition") not in DEFINITIONS):
        return None
    return c["id"]


def naive_max(case):
    """Intentionally unsafe largest-numeric-value comparator."""
    available = [c for c in case["claims"]
                 if type(c.get("value")) is int and c["value"] > 0]
    if not available:
        return None
    return max(available, key=lambda c: (c["value"], c["id"]))["id"]


def main():
    fixtures = cases()
    observations, violations = [], []
    metamorphic_trials = 0
    for case in fixtures:
        expected = case["expected"]
        strict = strict_select(case)
        naive = naive_max(case)
        observations.append(dict(case=case["id"], expected=expected,
                                 strict=strict, naive=naive))
        if strict != expected:
            violations.append("oracle: " + case["id"])
        for seed in range(20):
            shuffled = copy.deepcopy(case)
            random.Random(seed).shuffle(shuffled["claims"])
            metamorphic_trials += 1
            if strict_select(shuffled) != strict:
                violations.append("order: " + case["id"] + ":" + str(seed))
    additional = copy.deepcopy(fixtures[0])
    additional["claims"].append(claim(value=999999999, reviewed=False, claim_id="outlier"))
    metamorphic_trials += 1
    if strict_select(additional) != "c1":
        violations.append("added unreviewed outlier changed result")
    additional["claims"] = [c for c in additional["claims"] if c["id"] != "c1"]
    metamorphic_trials += 1
    if strict_select(additional) is not None:
        violations.append("lost preferred claim silently replaced")
    # Concrete targeted mutant: remove the requirement for an explicit date.
    year_guard_mutation_detected = (
        strict_select(fixtures[1]) is None
        and strict_select(fixtures[1], require_year=False) != fixtures[1]["expected"]
    )
    if not year_guard_mutation_detected:
        violations.append("missing-year mutant escaped")
    print(json.dumps(dict(
        id="EXP-FACTS-01",
        scope="Synthetic Python research model, not production runtime",
        cases=len(fixtures),
        strict_oracle_agreement=sum(x["strict"] == x["expected"] for x in observations),
        naive_oracle_agreement=sum(x["naive"] == x["expected"] for x in observations),
        metamorphic_checks=metamorphic_trials,
        year_guard_mutation_detected=year_guard_mutation_detected,
        input_sha256=hashlib.sha256(json.dumps(fixtures, sort_keys=True).encode()).hexdigest(),
        observations=observations, violations=violations,
    ), indent=2))
    return 1 if violations else 0


if __name__ == "__main__":
    raise SystemExit(main())
