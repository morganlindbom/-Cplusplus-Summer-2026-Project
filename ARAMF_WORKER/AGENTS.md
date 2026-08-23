<!-- AGENTS.md -->

# Canonical ARAMF Agent Instructions

Read `PROJECT_STATUS.md` and `memory/decisions.md` before project work.
Read `memory/framework-knowledge.json` and apply only entries whose status is `approved`.
Approved Framework Knowledge is live: it applies immediately in this project without regeneration.
Read `rules/generated-rules.md` when rule output is present.

Respect Sources of Truth, durable decisions, and the user-owned `custom/` directory.
Authority order: explicit current user instruction, current Source of Truth, current durable project decisions, approved Framework Knowledge, templates/defaults, then AI inference.
When a corrected approach is verified and reusable, record a Framework Knowledge candidate with evidence. Never self-approve it; explicit user approval is required before changing its status to `approved`. Superseded entries remain auditable but are not active.
Keep project status current and use project memory when configured.
The generated control directory is `ARAMF_WORKER/`.
Framework Knowledge has distinct built-in, global, and project-local layers. The global user library is stored under `ARAMF_DATA/` at the resolved ARAMF program root; build directories are disposable. Only explicitly approved portable knowledge may be promoted there; use the memory knowledge promotion command and never edit knowledge stores directly. New projects seed approved global knowledge without replacing project-local authority.
UPDATE is a separate human-controlled workflow: review approved Framework Knowledge, analyze the whole project, prepare a plan, then explicitly execute it through the configured agent. Read `update/update-plan.json` and `update/update-contract.json` when present; the managed project root is the implementation target and `ARAMF_WORKER/` is orchestration only. `READY_FOR_EXTERNAL_AGENT` is an incomplete handoff, not completion; actual project changes and validation are required. Preserve higher-authority instructions and use the scope-aware validation policy.
Run the minimum validation required by `routing/validation-policy.json`; do not run full regression campaigns for ordinary isolated changes. Escalate when scope, risk, failure, or explicit milestone policy requires it.
<!-- ARAMF-MEMORY-BEGIN -->

## Project Memory Feedback

Read `memory/memory-contract.json` before recording development results. Do not edit `memory/event-log.jsonl`, `memory/metrics.json`, `memory/current-state.md`, `memory/memory-manifest.json`, validation state, or `PROJECT_STATUS.md` bookkeeping fields directly. Use the ARAMF recorder described by the contract: `aramf memory record --project <project-root> --operation <operation> ...`.
- Record durable decisions only for genuine architecture or policy choices through the decision workflow.
- Follow current durable decisions; explicitly superseded decisions remain historical and inactive.

The recorder owns event IDs, timestamps, sequences, metrics, pruning, validation, and current-state pointers.

<!-- ARAMF-MEMORY-END -->





