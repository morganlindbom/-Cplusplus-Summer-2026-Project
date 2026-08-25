<!-- AGENTS.md -->

# Pico Visual Designer — Root Agent Instructions

This repository is governed by the project-local ARAMF control plane.

The canonical agent instructions are located at:

`ARAMF_WORKER/AGENTS.md`

Before substantial project work, every agent must:

1. Read this root `AGENTS.md`.
2. Read the complete `ARAMF_WORKER/AGENTS.md`.
3. Read `ARAMF_WORKER/PROJECT_STATUS.md`.
4. Read `ARAMF_WORKER/memory/decisions.md`.
5. Read `ARAMF_WORKER/memory/memory-contract.json`.
6. Read applicable approved Framework Knowledge and project Sources of Truth required by the canonical instructions.

Do not rely on cached, remembered, summarized, or previous versions of ARAMF instructions when the current project files are available.

## Authority

The detailed authority hierarchy is defined in `ARAMF_WORKER/AGENTS.md`.

Explicit current user instructions and the current project Source of Truth take precedence over older or generic ARAMF assumptions.

## PVD Project-Local Governance

Pico Visual Designer uses project-local ARAMF governance.

PVD must not require or search for:

* `aramf.exe`
* a machine-global `aramf` CLI
* a separate recorder executable
* an external ARAMF service

unless such a mechanism is explicitly present in the current PVD Source of Truth.

The absence of these mechanisms must not block normal PVD development or normal human-readable `PROJECT_STATUS.md` maintenance.

## Project Status

Keep `ARAMF_WORKER/PROJECT_STATUS.md` current according to the canonical instructions.

Normal verified human-readable status may be updated directly.

Do not fabricate machine-owned Project Memory metadata such as event IDs, sequence numbers, generated metrics, audit state, checkpoint IDs, or validation metadata.

## Administrative Authority

The established ARAMF administrator is:

**Admin Morgan Lindbom**

For Framework Knowledge operations:

`Authorized by: Admin Morgan Lindbom`

is sufficient administrative authorization according to the detailed rules in `ARAMF_WORKER/AGENTS.md`.

## Safety

TOP PRIORITY:

Never use broad destructive recursive cleanup such as:

* `rmdir /s /q`
* `Remove-Item -Recurse`
* `rm -rf`
* equivalent broad recursive deletion

Follow the complete safety policy in `ARAMF_WORKER/AGENTS.md`.

## Git

Do not commit or push unless explicitly instructed by the user.

## Canonical Rule

`ARAMF_WORKER/AGENTS.md` contains the complete project governance.

If this root routing file omits detail, follow the canonical instructions there.

Do not duplicate the full ARAMF policy into this root file.
