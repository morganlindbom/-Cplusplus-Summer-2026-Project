
<!-- AGENTS.md -->

# Canonical ARAMF Agent Instructions

Read `PROJECT_STATUS.md` and `memory/decisions.md` before project work.

Read `memory/framework-knowledge.json` and apply only entries whose status is `approved`.

Approved Framework Knowledge is live: it applies immediately in this project without regeneration.

Read `rules/generated-rules.md` when rule output is present.

Respect Sources of Truth, durable decisions, Framework Knowledge, Project Memory governance, and the user-owned `custom/` directory.

# Authority Order

Apply authority in this order:

1. Explicit current user instruction
2. Current project Source of Truth
3. Current durable project decisions
4. Applicable approved project-local Framework Knowledge
5. Applicable approved global Framework Knowledge
6. Templates/defaults
7. AI inference

Higher authority always overrides lower authority.

Global Framework Knowledge must never silently override explicit current user instructions, current project Source of Truth, or durable project decisions.

When a corrected approach is explicitly accepted, verified, and reusable, treat the corrected behavior as the current baseline.

Do not reintroduce superseded behavior unless newer evidence or higher authority requires it.

# Pico Visual Designer ARAMF Governance

For Pico Visual Designer, ARAMF governance is project-local.

The governing agent interface is this `AGENTS.md` together with the canonical files under:

`ARAMF_WORKER/`

PVD must not require a separate ARAMF executable, machine-global CLI, recorder executable, or external ARAMF service in order to perform ordinary governed development work.

In particular, PVD must not require or attempt to restore, install, create, or locate:

* `aramf.exe`
* a machine-global `aramf` command
* `recorder.exe`
* a separate Project Memory executable
* an external ARAMF memory service

unless such a component is explicitly present and authoritative in the current PVD Source of Truth.

The absence of such an external executable or service must not block:

* normal PVD development
* normal `PROJECT_STATUS.md` maintenance
* Framework Knowledge use
* Framework Knowledge fallback operations
* AGENTS.md-governed work
* project-local Project Memory behavior that is actually implemented

This PVD-specific project decision has higher authority than older or generic ARAMF assumptions about external recorder invocation.

# Project Status

Read `PROJECT_STATUS.md` before substantial continuation work.

Keep the human-readable project status current as verified work progresses.

Agents may directly update normal human-readable `PROJECT_STATUS.md` sections according to this `AGENTS.md`.

Normal project-status information includes:

* current implementation state
* completed work
* verified tests
* failed tests
* known failures
* blockers
* certification state
* current limitations
* next planned work
* verified project milestones

Normal project-status maintenance must not be blocked solely because no separate recorder executable, global CLI, or external ARAMF service exists.

Do not fabricate or manually alter machine-owned bookkeeping that belongs to a genuinely implemented project-local managed mechanism.

Machine-owned bookkeeping may include, where actually implemented:

* event IDs
* sequence numbers
* generated metrics
* audit chains
* anti-replay state
* generated validation metadata
* generated fingerprints
* machine-generated manifest counters

Human-readable project status and machine-owned Project Memory metadata are distinct responsibilities.

# ARAMF Control Plane

The generated ARAMF control directory is:

`ARAMF_WORKER/`

The managed PVD project root is the implementation target.

`ARAMF_WORKER/` is the ARAMF orchestration and governance layer and is not the normal PVD production-code implementation target.

The user-owned:

`custom/`

directory remains under user authority and must not be modified automatically unless explicitly instructed.

Do not create parallel ARAMF control planes merely to work around a missing mechanism.

# Framework Knowledge

Framework Knowledge represents approved reusable knowledge.

Framework Knowledge and Project Memory are separate systems and must remain conceptually distinct.

Framework Knowledge has three layers:

1. built-in
2. global
3. project-local

## Built-In Framework Knowledge

Built-in Framework Knowledge is ARAMF-provided source material and defaults.

It must not override stronger project authority.

## Global Framework Knowledge

Global Framework Knowledge represents approved portable knowledge intended to apply across managed projects where relevant.

Where the current ARAMF architecture provides a global knowledge library, use that established storage model.

Build directories must never become accidental owners of persistent global knowledge.

Only explicitly approved portable knowledge belongs in the global Framework Knowledge layer.

Global knowledge remains subordinate to stronger current project authority.

## Project-Local Framework Knowledge

Project-local Framework Knowledge is stored in:

`memory/framework-knowledge.json`

inside the project's `ARAMF_WORKER`.

Project-local approved knowledge applies to the current project.

Globally approved knowledge may also be seeded or adopted into project-local storage according to the active ARAMF Framework Knowledge workflow.

## Framework Knowledge Candidate Workflow

When a corrected approach, reusable lesson, testing principle, architecture lesson, safety rule, or other generalizable knowledge has been verified, agents may create a Framework Knowledge candidate with evidence.

Agents must not self-approve ordinary Framework Knowledge.

Explicit user approval is normally required before changing a candidate to approved state.

Superseded entries remain durable and auditable but are inactive.

Do not erase historical knowledge merely because a newer record supersedes it.

## Framework Knowledge Promotion

Framework Knowledge promotion to the global layer is distinct from ordinary project approval.

Do not assume project approval automatically means global promotion.

Preserve stable IDs, provenance, portability, semantic duplicate protection, and higher-authority project rules.

# Administrative Framework Knowledge Authorization

## Established Administrator

The established ARAMF administrator is:

**Admin Morgan Lindbom**

This identity is authoritative for ARAMF administrative Framework Knowledge operations defined by the active project contract.

## Sufficient Administrative Authorization

For approved ARAMF Framework Knowledge create or update operations, an instruction that explicitly states:

`Authorized by: Admin Morgan Lindbom`

is sufficient administrative authorization.

No additional specially worded one-time authorization is required for each individual Framework Knowledge record.

Do not block a legitimate Framework Knowledge operation solely because another custom authorization statement was not supplied.

The exact administrator identity must remain:

`Admin Morgan Lindbom`

Do not infer another identity as equivalent.

## Administrative Framework Knowledge Scope

Admin Morgan Lindbom may authorize Framework Knowledge operations for applicable ARAMF scopes, including existing equivalents of:

* `PROJECT_LOCAL`
* `GLOBAL_FRAMEWORK`

Global Framework Knowledge remains subordinate to:

1. explicit current user instructions
2. current project Source of Truth
3. current durable project decisions
4. applicable stronger project-local authority

Administrative approval must not be interpreted as permission for global knowledge to override stronger project authority.

## Preferred Framework Knowledge Path

When a canonical project-local Framework Knowledge mechanism exists and is operational, use it.

The conceptual lifecycle is:

`user/admin approval`
→ `project-local ARAMF governance`
→ `persistence`
→ `validation`
→ `later rediscovery`

This does not imply the existence of an external executable, machine-global CLI, or external service.

## Authorized Framework Knowledge Fallback

If a specialized Framework Knowledge mechanism is unavailable, a controlled direct Framework Knowledge fallback is authorized when the operation explicitly states:

`Authorized by: Admin Morgan Lindbom`

Under this fallback, direct creation or update of:

`memory/framework-knowledge.json`

is permitted for Framework Knowledge operations only.

This is an explicitly authorized ARAMF administrative fallback and must not be reported as an unauthorized protected-file bypass.

## Framework Knowledge Fallback Requirements

When using the Admin Morgan Lindbom fallback:

* use the existing canonical `framework-knowledge.json` schema
* do not create a competing knowledge store
* do not create a competing schema
* preserve unrelated entries
* preserve stable IDs when updating existing entries
* perform semantic duplicate checks before creating new entries
* do not create duplicate semantic knowledge
* preserve approved state, origin, portability, and unrelated metadata unless the authorized operation explicitly changes them
* generate stable ARAMF-style IDs for genuinely new entries
* write valid JSON
* reload and parse the file after writing
* verify the created or updated entry exists exactly once
* verify unrelated pre-existing entries remain present
* report the exact mutation

If audit, anti-replay, memory consistency, cold-start validation, or specialized rediscovery mechanisms do not exist for the operation, report them as:

`NOT AVAILABLE`

Do not fabricate PASS results.

If a narrow temporary backup is required, use a safe recoverable backup.

## Framework Knowledge Fallback Restriction

The Framework Knowledge fallback does **not** grant unrestricted direct modification of Project Memory machine metadata.

It does not authorize fabricated editing of:

* `memory/event-log.jsonl`
* machine-generated metrics
* machine-generated manifests
* checkpoint identifiers
* event sequence numbers
* audit chains
* anti-replay state
* generated validation metadata

Normal human-readable `PROJECT_STATUS.md` maintenance is not prohibited by this restriction.

## Session Memory Is Not Durable Framework Knowledge

Agent memory, chat history, conversation history, or the fact that an agent understood an instruction is not equivalent to durable ARAMF Framework Knowledge.

Reusable approved knowledge must exist in the canonical project knowledge storage.

# UPDATE Workflow

UPDATE remains a separate human-controlled Framework Knowledge workflow.

The normal sequence is:

1. review approved Framework Knowledge
2. analyze the complete managed project
3. determine applicability
4. prepare a plan
5. explicitly execute through the configured agent
6. validate resulting project changes

Read:

* `update/update-plan.json`
* `update/update-contract.json`

when present.

Prepare is not implementation completion.

`READY_FOR_EXTERNAL_AGENT` is an incomplete handoff state.

Actual project changes and applicable validation are required before completion may be claimed.

Preserve higher-authority instructions throughout UPDATE.

Do not mutate project knowledge during analysis or preparation unless the active lifecycle explicitly requires it.

# Validation Routing

Use:

`routing/validation-policy.json`

when present.

Run the minimum validation justified by scope and risk.

Do not run full regression campaigns for ordinary isolated changes.

Escalate validation when justified by:

* larger changed scope
* higher risk
* observed failure
* uncertainty
* milestone
* release
* migration
* explicit certification
* dependency impact

Do not claim validation that was not actually executed.

<!-- ARAMF-MEMORY-BEGIN -->

# Project Memory Feedback

Read:

`memory/memory-contract.json`

before substantial project work and before recording or interpreting Project Memory state.

For Pico Visual Designer, Project Memory governance is project-local and AGENTS.md-driven.

Project Memory must not depend on the existence of a separate ARAMF executable, machine-global CLI, recorder executable, or external ARAMF service.

## PVD Managed-Mechanism Terminology

Within Pico Visual Designer, references in ARAMF instructions to:

* official recorder
* official retrieval mechanism
* managed mechanism
* Project Memory mechanism
* recorder
* retrieval path

mean:

**the canonical project-local mechanism defined by the active PVD ARAMF contract, this AGENTS.md, and the actual project Source of Truth.**

These terms do **not** imply that PVD must have:

* `aramf.exe`
* a machine-global `aramf` command
* a separate recorder executable
* an external memory process
* an external ARAMF service

unless such a mechanism is explicitly implemented and authoritative in the current PVD Source of Truth.

Do not search for, require, restore, create, or install an external recorder merely because generic ARAMF wording uses terms such as "official recorder" or "managed mechanism."

A genuinely implemented specialized project-local mechanism may be used when it exists.

If such a specialized mechanism is unavailable, report:

`NOT AVAILABLE`

where appropriate.

Its absence must not block ordinary PVD development or normal human-readable `PROJECT_STATUS.md` maintenance.

# Project Memory Ownership

Project Memory may contain both human-maintained state and machine-owned bookkeeping.

Do not fabricate machine-owned Project Memory metadata.

Where a genuinely implemented project-local managed mechanism owns data such as:

* event IDs
* timestamps
* sequence numbers
* event ordering
* generated metrics
* audit chains
* anti-replay state
* generated manifests
* generated validation metadata
* generated fingerprints
* checkpoint IDs

allow that mechanism to own those fields.

Do not manually invent values merely to make Project Memory appear complete.

# Mandatory Development Memory

When Project Memory is enabled by the active PVD project contract, agents must preserve meaningful development history and project state as work occurs.

This requirement does not imply an external recorder.

Use the project-local mechanism that actually exists.

If only human-readable project status is available for a particular kind of information, maintain that human-readable state truthfully.

Do not fabricate machine-generated events merely because an event type conceptually exists.

## Task Start

Before beginning a distinct substantial unit of work, establish the current task in the available governed project state.

Examples include:

* implementation
* correction
* investigation
* diagnosis
* refactor
* architecture work
* significant testing
* certification work

If a genuine project-local event mechanism exists, use it.

If no such event mechanism exists, do not fabricate event metadata.

The absence of an event mechanism must not block the task.

## Build Results

Preserve meaningful production build outcomes.

Record both successful and failed build attempts in the governed project state where the active project mechanisms support this.

At minimum, verified meaningful build state should be reflected in normal project status when relevant.

Do not invent:

* event IDs
* sequence numbers
* generated metrics
* audit entries

merely to represent a build result.

## Test Results

Preserve meaningful test executions and outcomes.

Record both PASS and FAIL results where applicable.

Where available include:

* suite
* passed count
* failed count
* total
* relevant detail

Use canonical project-local mechanisms when they exist.

Otherwise maintain truthful human-readable project status without fabricating machine bookkeeping.

## Validation Results

Preserve meaningful:

* validation
* verification
* certification
* memory checks
* subsystem acceptance
* milestone acceptance

Do not claim a validation that did not actually run.

If no specialized validation-recording mechanism exists, the verified result may still be reflected in normal human-readable project status.

## Task Completion

A task is complete only when the requested work and applicable validation boundary are actually complete.

Do not claim completion merely because source code changed.

If work is:

* partial
* blocked
* awaiting validation
* awaiting physical verification
* awaiting external equipment

preserve the real state.

Do not fabricate a machine-generated completion event when no such mechanism exists.

# Normal Project Memory Lifecycle

Conceptually, substantial work may contain:

`task start`

→ implementation / investigation / diagnosis

→ build results

→ test results

→ validation results where applicable

→ corrective work where required

→ further verification

→ task completion

This conceptual lifecycle must be preserved.

However, its existence does not require an external ARAMF executable or CLI.

Use only mechanisms that genuinely exist in the PVD project Source of Truth.

# Project-Local Memory Invocation

AGENTS.md is the primary governing agent interface for PVD.

Use canonical project-local mechanisms defined by:

* this `AGENTS.md`
* `memory/memory-contract.json`
* the active project memory configuration
* current project Source of Truth

Do not assume, search for, require, create, restore, or install:

* `aramf.exe`
* a global `aramf` CLI
* a separate Project Memory recorder executable
* an external ARAMF memory service

A specialized project-local mechanism may be used if it genuinely exists in the current project Source of Truth.

If no such specialized mechanism exists:

* do not fabricate event IDs
* do not fabricate sequence numbers
* do not fabricate audit records
* do not fabricate checkpoints
* do not fabricate anti-replay state
* do not fabricate generated metrics
* do not fabricate validation results
* report the specialized mechanism as `NOT AVAILABLE` where relevant

The absence of a specialized Project Memory mechanism must not block:

* ordinary PVD development
* normal human-readable project-status maintenance
* Framework Knowledge use
* AGENTS.md-governed work

# Durable Decisions

Durable decisions represent genuine long-lived project authority.

Examples include:

* architecture
* ownership
* lifecycle policy
* safety policy
* Source-of-Truth policy
* persistent technical constraints

Use the canonical project-local decision mechanism defined by the current PVD ARAMF control plane.

Current decisions are authoritative at their priority level.

Superseded decisions remain historical and auditable but are inactive.

Ordinary implementation details, temporary diagnostics, experimental attempts, and routine test observations are not automatically durable decisions.

Do not fabricate machine-generated decision bookkeeping if no such mechanism exists.

# Checkpoints

Checkpoints represent deliberate stable recovery points.

Appropriate uses include:

* stable verified baseline
* significant recovery point
* explicit milestone
* major validated transition
* handoff-worthy state

Do not create checkpoints for every routine development action.

Do not fabricate checkpoint IDs or machine metadata when no checkpoint mechanism exists.

A checkpoint does not replace:

* normal project status
* development history
* durable decisions
* Framework Knowledge

# Project Memory Rediscovery

Before substantial continuation work, especially after:

* fresh agent session
* project reopen
* process restart
* agent handoff
* interruption
* cold start
* long development gap

recover relevant state from the canonical project-local sources.

These may include:

* `PROJECT_STATUS.md`
* `memory/decisions.md`
* available Project Memory artifacts
* available checkpoints
* current project configuration
* current Source of Truth
* approved Framework Knowledge

Use a specialized project-local retrieval mechanism when one genuinely exists.

Do not require an external executable or global CLI merely to perform rediscovery.

Do not rely only on chat/session history.

# Event Rediscovery

Where a canonical project-local event-history mechanism exists, use it to understand prior development history.

Where no specialized retrieval mechanism exists, inspect the canonical project-local governed state that is actually available.

Do not fabricate history.

Do not infer that events are absent merely because no global ARAMF command exists.

# Decision Rediscovery

Before changing:

* architecture
* policy
* ownership
* safety behavior
* lifecycle rules
* Source-of-Truth rules
* other durable behavior

read applicable current durable decisions.

Distinguish:

* current
* superseded

Never treat superseded decisions as current authority.

# Checkpoint Rediscovery

Where checkpoints are implemented, use them as recovery aids.

Retrieve applicable:

* checkpoint identity
* verification state
* relevant project sequence/state

only through mechanisms that genuinely exist.

Do not invent missing checkpoint metadata.

# Cold-Start Recovery

Cold-start recovery means that the project can reconstruct the necessary governed state after restart or a fresh agent session.

For PVD, this does not require an external ARAMF process.

The canonical recovery basis includes the project-local ARAMF control plane and current Source of Truth.

Where specialized semantic recovery mechanisms exist, use them.

Where they do not exist, report them as:

`NOT AVAILABLE`

Do not claim semantic validation that was not actually performed.

# recordingEnabled

For Pico Visual Designer:

`recordingEnabled`

describes whether the PVD Project Memory workflow defined by the active project contract is enabled and operational.

It does **not** imply the existence of:

* an external executable
* global CLI
* separate recorder
* external service

Resolve its meaning from:

1. `memory/memory-contract.json`
2. the active project memory configuration
3. project-local mechanisms defined by this `AGENTS.md`
4. current project Source of Truth

Do not infer that Project Memory is disabled merely because no global `aramf` command or external recorder exists.

Do not manually flip `recordingEnabled` merely to manufacture capability.

Its value must represent the actual configured PVD Project Memory workflow.

# Failure and Historical Evidence

Failed:

* builds
* tests
* validations
* certification attempts
* diagnostic attempts
* implementation approaches
* external-tool operations

are valid historical evidence when meaningful.

Preserve meaningful failures.

Do not erase or rewrite an earlier failure because a later attempt succeeded.

Where possible preserve:

* task
* failure domain
* stage
* actual result
* correction
* later retest result

Use normal human-readable project status when appropriate.

Do not fabricate machine metadata to represent historical evidence.

# PROJECT_STATUS Maintenance

Agents may directly update normal human-readable `PROJECT_STATUS.md` sections according to this `AGENTS.md`.

This includes verified information such as:

* implementation status
* completed work
* pending work
* build results
* test results
* validation results
* certification state
* failures
* blockers
* limitations
* next steps

Normal `PROJECT_STATUS.md` maintenance must never be blocked solely because:

* `aramf.exe` does not exist
* no global `aramf` command exists
* no separate recorder exists
* no external ARAMF service exists

Do not fabricate or manually alter machine-owned bookkeeping that belongs to a genuinely implemented project-local managed mechanism.

The distinction is:

**Human-readable verified project status may be maintained directly.**

**Machine-generated bookkeeping must not be fabricated.**

# Project Memory vs Durable Decisions vs Checkpoints vs Framework Knowledge

Keep these concepts distinct.

A Project Memory event answers:

**"What happened in this project?"**

A durable decision answers:

**"What long-lived project choice is authoritative?"**

A checkpoint answers:

**"What verified recovery point should be preserved?"**

Framework Knowledge answers:

**"What approved reusable knowledge should apply to future applicable work?"**

Do not collapse these concepts into one generic memory mechanism.

Do not use Framework Knowledge as a substitute for routine project history.

Do not use ordinary project-history notes as a substitute for durable decisions.

Do not use checkpoints as a substitute for project status.

# Required Agent Behavior

For substantial PVD work:

* read the current governed project state first
* follow the active authority order
* preserve meaningful development history
* maintain normal human-readable `PROJECT_STATUS.md`
* preserve genuine durable decisions
* use checkpoints deliberately
* apply approved Framework Knowledge
* run applicable validation
* preserve failed attempts
* do not fabricate machine metadata
* do not block normal work solely because an external recorder or global CLI does not exist

If a specialized project-local Project Memory mechanism exists, use it according to the active contract.

If it does not exist, report that specialized function as:

`NOT AVAILABLE`

and continue normal governed PVD development through the mechanisms that actually exist.

<!-- ARAMF-MEMORY-END -->

# Safety

## TOP PRIORITY — No Destructive Recursive Cleanup

ARAMF-managed agents and tools must never use broad recursive shell deletion for cleanup or fixture removal.

Forbidden examples include:

* `cmd.exe /c rmdir /s /q`
* `rmdir /s /q`
* `rd /s /q`
* PowerShell `Remove-Item -Recurse`
* Unix `rm -rf`
* equivalent broad recursive deletion commands

Repositories, project roots, `ARAMF_WORKER`, persistent knowledge, generated persistent state, build trees containing valuable state, and unrelated workspaces must never be broadly deleted for cleanup.

Malformed quoting, path resolution, shell interpolation, or variable expansion can widen deletion scope.

Before removing anything:

1. resolve the exact target
2. verify target ownership
3. verify target scope
4. prefer non-destructive or recoverable cleanup
5. preserve unrelated state

Failed cleanup attempts and safety incidents remain historical evidence.

# Agent Completion Rules

Before claiming completion:

* confirm the requested work was actually performed
* run applicable validation
* update normal human-readable `PROJECT_STATUS.md`
* preserve meaningful failures
* preserve current durable decisions
* preserve Framework Knowledge authority
* preserve higher-priority user instructions
* report anything not run as `NOT RUN` or `NOT AVAILABLE`
* do not claim PASS for unavailable validation
* do not fabricate machine-generated Project Memory metadata
* do not block ordinary PVD development because no external ARAMF executable exists
* do not commit or push unless explicitly authorized

If work is blocked, report the exact real blocking boundary.

Do not manufacture a successful result by inventing Project Memory events, metrics, checkpoints, audit records, sequence numbers, anti-replay state, or validation evidence.
