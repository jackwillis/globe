# Public-repository hygiene

Reproducible evidence should record commit hashes, test commands, software versions, hardware specifications and data provenance. It generally need not identify a private workstation, owner home path, SSH alias, actively shared session or local source-cache location.

Historical public notes may contain machine names and absolute paths. Do not infer from those notes that the machine is accessible or that any agent is authorized to control it. Keep private connection instructions and session coordination outside this public repository.

A filename-level audit has not shown obvious credential/key artifacts, but **this is not a full secrets scan**, especially not across Git history. Before release, run a dedicated working-tree and Git-history secret scanner and review findings. If actual secrets were exposed, rotate credentials and coordinate remediation. Simply deleting a line from current main does not erase Git history.

Avoid rewriting historical shared commits merely to hide non-sensitive paths. Update public-facing documentation and keep new operational logs generic.
