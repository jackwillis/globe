# Review correction evidence — 2026-11-18

No geographic quality scores or performance measurements were run. B is unchanged.

## Synthetic numerical controls

Errors use normalized encoded RGB. Support MAE includes pixels with nonzero continuous reference; max is full-frame worst pixel-channel error. All open endpoints remain included. Values are approximation characterization, not geographic acceptance.

| Case | Support MAE | Max | Quantized MAE | Sequential MAE | Ink ratio |
|---|---:|---:|---:|---:|---:|
| octant0-length0.25 | 0.140878 | 0.280190 | 0.137255 | 0.133987 | 2.1020 |
| octant0-length0.5 | 0.198366 | 0.274441 | 0.205882 | 0.205882 | 1.7935 |
| octant0-length1 | 0.072890 | 0.183324 | 0.071569 | 0.071569 | 1.2536 |
| octant0-length3 | 0.027323 | 0.078823 | 0.023856 | 0.021242 | 1.0575 |
| octant0-length16 | 0.017023 | 0.078823 | 0.014173 | 0.014861 | 1.0046 |
| octant1-length0.25 | 0.248856 | 0.398294 | 0.257516 | 0.257516 | 2.9908 |
| octant1-length0.5 | 0.078560 | 0.192117 | 0.077451 | 0.074837 | 1.4954 |
| octant1-length1 | 0.053339 | 0.117654 | 0.048366 | 0.047320 | 1.2327 |
| octant1-length3 | 0.036289 | 0.117654 | 0.036020 | 0.035439 | 1.0749 |
| octant1-length16 | 0.020154 | 0.117654 | 0.018937 | 0.019072 | 1.0102 |
| octant2-length0.25 | 0.227941 | 0.239920 | 0.228758 | 0.228758 | 2.8235 |
| octant2-length0.5 | 0.121786 | 0.135457 | 0.126797 | 0.126797 | 1.7307 |
| octant2-length1 | 0.095098 | 0.129751 | 0.101307 | 0.101307 | 1.3804 |
| octant2-length3 | 0.054963 | 0.211356 | 0.057807 | 0.057807 | 1.1246 |
| octant2-length16 | 0.024840 | 0.197623 | 0.024937 | 0.024401 | 1.0262 |
| octant3-length0.25 | 0.028297 | 0.067280 | 0.030937 | 0.030937 | 1.3020 |
| octant3-length0.5 | 0.030848 | 0.080525 | 0.030065 | 0.025817 | 1.1137 |
| octant3-length1 | 0.094007 | 0.210180 | 0.097386 | 0.094444 | 1.3255 |
| octant3-length3 | 0.023300 | 0.061280 | 0.020915 | 0.021932 | 1.0148 |
| octant3-length16 | 0.016709 | 0.052552 | 0.016088 | 0.015619 | 0.9968 |
| octant4-length0.25 | 0.028297 | 0.067280 | 0.030937 | 0.030937 | 1.3020 |
| octant4-length0.5 | 0.030848 | 0.080525 | 0.030065 | 0.025817 | 1.1137 |
| octant4-length1 | 0.094007 | 0.210180 | 0.097386 | 0.094444 | 1.3255 |
| octant4-length3 | 0.023300 | 0.061280 | 0.020915 | 0.021932 | 1.0148 |
| octant4-length16 | 0.016709 | 0.052552 | 0.016088 | 0.015619 | 0.9968 |
| octant5-length0.25 | 0.227941 | 0.239920 | 0.228758 | 0.228758 | 2.8235 |
| octant5-length0.5 | 0.121786 | 0.135457 | 0.126797 | 0.126797 | 1.7307 |
| octant5-length1 | 0.095098 | 0.129751 | 0.101307 | 0.101307 | 1.3804 |
| octant5-length3 | 0.054963 | 0.211356 | 0.057807 | 0.057807 | 1.1246 |
| octant5-length16 | 0.024840 | 0.197623 | 0.024937 | 0.024401 | 1.0262 |
| octant6-length0.25 | 0.248856 | 0.398294 | 0.257516 | 0.257516 | 2.9908 |
| octant6-length0.5 | 0.078560 | 0.192117 | 0.077451 | 0.074837 | 1.4954 |
| octant6-length1 | 0.053339 | 0.117654 | 0.048366 | 0.047320 | 1.2327 |
| octant6-length3 | 0.036289 | 0.117654 | 0.036020 | 0.035439 | 1.0749 |
| octant6-length16 | 0.020154 | 0.117654 | 0.018937 | 0.019072 | 1.0102 |
| octant7-length0.25 | 0.140878 | 0.280190 | 0.137255 | 0.133987 | 2.1020 |
| octant7-length0.5 | 0.198366 | 0.274441 | 0.205882 | 0.205882 | 1.7935 |
| octant7-length1 | 0.072890 | 0.183324 | 0.071569 | 0.071569 | 1.2536 |
| octant7-length3 | 0.027323 | 0.078823 | 0.023856 | 0.021242 | 1.0575 |
| octant7-length16 | 0.017023 | 0.078823 | 0.014173 | 0.014861 | 1.0046 |
| join15 | 0.013254 | 0.195957 | 0.010784 | 0.011811 | 0.9884 |
| join45 | 0.024850 | 0.223510 | 0.025224 | 0.025224 | 1.0170 |
| join90 | 0.017242 | 0.142647 | 0.011817 | 0.011817 | 1.0195 |
| join150 | 0.021671 | 0.100262 | 0.020312 | 0.020312 | 1.0101 |
| closed-square | 0.014412 | 0.253513 | 0.009877 | 0.009877 | 0.9937 |
| closed-ring | 0.019124 | 0.085791 | 0.016630 | 0.015323 | 1.0067 |
| arc20-chord0.25 | 0.041665 | 0.114295 | 0.045349 | 0.045349 | 1.0938 |
| arc20-chord0.5 | 0.035695 | 0.134922 | 0.035566 | 0.036438 | 1.0134 |
| arc20-chord1 | 0.027587 | 0.138661 | 0.024953 | 0.025069 | 0.9995 |
| arc20-chord3 | 0.022831 | 0.181278 | 0.021927 | 0.021927 | 0.9939 |

50 cases; zero missing pixels where reference coverage >=25%. Final accepted tile comparison maximum 0.0009765625 (<=1/1024). Maximum sampling level 512. Full per-tile refinement history is in synthetic-convergence.json. No 5%-of-verdict-margin claim is made from these synthetic diagnostics.

## Structural operation counts

| Map fixtures (16 geometries) | Min | Median | Max |
|---|---:|---:|---:|
| segments | 5190 | 10024.0 | 17347 |
| joins | 5105 | 9679.0 | 17002 |
| primitive_pixel_evaluations | 62402 | 285243.0 | 415178 |
| feature_bbox_pixels | 495546 | 851388.5 | 1906715 |
| max_segment_bbox | 42 | 42.0 | 42 |

Feature bbox area is cleared once and scanned once; count those traversals separately. Primitive evaluations include joins and rectangles. These counts are not latency measurements or predictions.

## Provenance limitation

The pre-image hashes of reference.py and test_aa.cpp differ from the original final prototype. Neither matching pre-image content was found among reachable or unreachable objects in the private scratch Git store (git fsck --no-reflogs --unreachable, SHA256 comparison of blobs <=100000 bytes). The original pre-image freeze and final files remain preserved. No diff or reliable rationale can be recovered; in particular, no claim that reference semantics were unchanged is supported. Earlier mention of a test compile fix does not establish the reference.py change. This is an audit gap. A new verified source freeze supersedes the instrument freeze for future work; original images remain valid as unchanged B/extractor evidence.

aa.cpp, aa.hpp, fixtures.cpp and images.py match both the pre-image hashes and original final prototype exactly. reference.py and test_aa.cpp are left unchanged by this correction; new independent instruments are separate files. No B tuning occurred.

## Remaining approvals and limitations

Owner review and explicit scoring authorization remain required. Adaptive maximum added-line and total-settle millisecond budgets on a named platform remain UNSET and require owner approval before timing. Interactive replacement retains the original 25% line-latency gate. Timed runner implementation/review and oracle convergence against actual decision margins remain prerequisites. No G4/Classic acceptance, PR, merge, release or automation.

Retro68 probe was unavailable: podman/docker failed before container startup because /run/user/1000/libpod required a sticky-bit change on a read-only filesystem. No permissions were changed. The script does not include aa.cpp by default; an added compile-only aa.cpp probe remains outstanding.

GitHub shell access failed; connected GitHub REST tools reached issue #25. Full milestone/ready/PR inventory and optional Project access were unavailable through shell and not reconstructed from old handoffs.
