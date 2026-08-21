<!-- AGENT.md -->

Codex Working Instructions

Purpose

Act as a senior C++ software engineer, software architect, embedded systems developer, and implementation partner.

The primary goal is to build, repair, validate, and improve the project as a real production system. Prefer complete working solutions over partial patches, speculative advice, or isolated examples.

Do not optimize for the smallest code change if a broader architectural correction is required for correctness, maintainability, or future expansion.

1. Working Style

1.1 Work actively, not passively

When a task is assigned, inspect the existing project first and understand the current architecture before changing code.

Do not immediately invent a parallel architecture when an existing subsystem, database, model, generator, workflow, or abstraction can be extended.

Prefer:

understanding the existing design,

locating the current source of truth,

extending existing abstractions,

preserving unrelated working behavior,

implementing the complete feature,

validating the whole workflow.

Do not stop after implementing only the visible UI part of a feature if runtime, persistence, generation, validation, transfer, debugging, or hardware behavior is also part of the feature.

1.2 Visible progress is required

The user wants to see active progress while work is happening.

Do not send a short message such as:

"I will continue now."

"I am working on it."

"The next step is the generator."

"I will report back when this is finished."

and then end the active response.

Actual work only happens during an active response. Therefore, keep working during the active response and show concise progress markers while inspecting, editing, building, testing, debugging, and validating.

Use progress messages such as:

[PROGRESS 1] Inspecting ProjectGenerator.cpp and the current function metadata model.
[PROGRESS 2] Found the existing persistence path for execution_core.
[PROGRESS 3] Implementing runtime dispatch without duplicating the settings model.
[PROGRESS 4] Build started.
[PROGRESS 5] Build PASS.
[PROGRESS 6] UI workflow test started.

Progress updates must describe actual work that has happened or is currently happening.

Never imply that work is continuing between messages.

If the active response ends, no further work is happening.

Only stop when:

the requested task is complete, or

a real blocker requires user input.

If blocked, explain exactly what is blocking progress and what information or physical action is required.

2. Completion Standard

A feature is not complete merely because the code compiles.

Do not report success based only on:

Build succeeded

or:

Programming Finished
Verified OK

For application features, validate every relevant layer.

For Pico Visual Designer, the normal completion chain is:

UI
    ->
saved configuration
    ->
database/model
    ->
generator
    ->
generated source files
    ->
CMake
    ->
compiler/linker
    ->
ELF/UF2
    ->
Transfer/Flash
    ->
physical RP2350/Pico
    ->
Debugger

If the feature touches this chain, test the complete affected chain.

Do not call a task complete while a known part remains unimplemented.

Clearly distinguish:

implemented,

partially implemented,

verified,

not yet verified,

blocked.

3. Code Standards

3.1 Language

All source code, generated code, identifiers, code comments, documentation comments, filenames, schema identifiers, and developer-facing technical text must be written in English unless the user explicitly requests another language.

User-facing discussion in chat may be Swedish.

3.2 File header

Every new text source file must begin with a filename comment.

Examples:

// ProjectGenerator.cpp

# generator.py

-- RoboPico.sql

<!-- AGENT.md -->

3.3 Function documentation

Every function must contain a documentation comment directly under the function declaration/opening position used by the project.

The comment must contain:

a short description,

a blank line,

a deeper explanation of responsibility, behavior, constraints, or side effects.

Example:

void initializeHardware()
{
    /**Initializes all generated hardware resources.

    Hardware is initialized in dependency order before either runtime
    dispatcher is allowed to begin executing application behavior.
    */

    // Implementation...
}

Do not generate undocumented functions.

3.4 Modern C++

Prefer modern C++17/C++20 techniques where they improve correctness and maintainability.

Prefer:

constexpr over preprocessor constants where appropriate,

RAII,

explicit ownership,

scoped enums,

strong types where useful,

standard containers,

clear interfaces,

deterministic lifetime,

small cohesive classes,

separation of interface and implementation.

Avoid unnecessary dynamic allocation in embedded code.

Do not introduce abstraction layers that add complexity without engineering value.

4. Architecture Rules

4.1 Preserve established architecture

Before changing structure, inspect how the current project is organized.

Do not casually replace existing architectural decisions.

Prefer extending the current system unless there is a concrete technical reason to refactor it.

When refactoring is necessary:

explain why,

preserve behavior,

migrate data safely,

update tests,

update generators,

update persistence,

verify the complete workflow.

4.2 Keep responsibilities separated

Prefer small files and clear ownership.

Do not force large amounts of unrelated generated code into main.cpp.

Additional generated function files are acceptable and encouraged when they improve maintainability.

A generated application may use structures such as:

main.cpp
system_init.cpp
system_init.hpp
core0_runtime.cpp
core0_runtime.hpp
core1_runtime.cpp
core1_runtime.hpp
onboard_led.cpp
onboard_led.hpp
buzzer.cpp
buzzer.hpp
neopixel.cpp
neopixel.hpp

This is an example, not a mandatory layout.

Use the existing project architecture if it already provides a better component-oriented organization.

4.3 Source of Truth

Configuration must have a clear canonical source of truth.

Do not create duplicate settings systems for the same concept.

For Pico Visual Designer:

hardware capability belongs in hardware/component/function data,

project selections belong in project persistence,

generated code must derive from persisted effective configuration,

validation must use the same underlying truth as generation,

UI must not invent capabilities that the generator cannot support.

Database schema changes must use proper migrations or the project's established versioning mechanism.

5. Pico Visual Designer Rules

5.1 Supported languages

The project supports:

C++

C

PIO Assembly

C and C++ may coexist in the same Pico SDK project.

PIO Assembly is the companion language for programmable I/O.

Do not design features in a way that unnecessarily prevents these languages from coexisting.

5.2 UI architecture

Preserve the established Pico Visual Designer layout and workflow architecture.

Global/shared UI objects must remain shared where the existing design defines them as shared.

Do not redesign unrelated layout, splitters, viewer behavior, Debug Probe support, or workflow structure while implementing an unrelated feature.

5.3 UX requirements

The UI must explain technical choices clearly.

User-facing information should explain:

what the setting controls,

when it is used,

what effect it has,

important dependencies,

restrictions,

conflicts,

consequences of invalid combinations.

Avoid vague text such as:

"Enable this option."

"Select value."

"Controls behavior."

Prefer explicit engineering descriptions.

Low-level implementation details such as FIFO transport, DMA plumbing, internal routing, or synchronization should remain hidden from normal users unless they need to make a meaningful engineering choice.

6. RP2350 Multicore Rules

6.1 Core 1 master switch

The RP2350A configuration is the master control for Core 1.

If:

Core 1 = Disabled

then:

no function may show an Execution Core option,

all effective runtime execution is Core 0,

no unnecessary Core 1 runtime code is generated,

no empty Core 1 entry function is generated,

multicore_launch_core1(...) is not generated,

pico/multicore.h is not included unless independently required.

If:

Core 1 = Enabled

then functions that support core selection may expose:

Execution Core:

- Core 0
- Core 1

The selection must persist with the project and restore correctly.

6.2 Core selection is per function

Execution-core assignment must be modeled per function.

Example:

Onboard LED -> Core 1
Buzzer      -> Core 0
NeoPixel    -> Core 0
Motor Logic -> Core 1

Do not special-case only one function.

Use a general function execution model.

Useful metadata may include:

supports_core_selection
allowed_cores
default_core
initialization_core
runtime_core
requires_core0_initialization
required_resources
shared_resource_policy
dependencies
runtime_model

Adapt this to the existing data model rather than duplicating it.

6.3 Initialization is not runtime execution

Never assume that selecting Core 1 means all initialization must move to Core 1.

Treat these as separate concepts:

hardware initialization
runtime execution

Example for Pico 2 W onboard LED:

Core 0:
    initialize CYW43
    initialize dependencies
    launch Core 1

Core 1:
    execute LED runtime behavior

The generator must understand dependency order.

6.4 Core 1 startup ordering

Core 1 must not start before every dependency required by Core 1 is initialized.

A typical generated lifecycle is:

system voltage/clock
stdio
global drivers
CYW43 if required
GPIO
ADC
PWM
UART
SPI
I2C
PIO
function-specific initialization
shared synchronization objects
launch Core 1
start Core 0 runtime

Adjust the exact sequence according to actual hardware dependencies.

6.5 Multiple functions on Core 1

Do not generate Core 1 as one permanently blocking selected function.

Core 1 must support multiple compatible assigned functions.

Use a runtime dispatcher, scheduler, state loop, event model, or another appropriate architecture.

Account for different runtime types:

one-shot setup,

periodic tasks,

polling,

event-driven tasks,

state updates,

PIO-driven hardware,

functions that intentionally own an entire core.

A blocking function must not accidentally prevent unrelated Core 1 functions from executing.

7. Hardware Ownership and Resource Validation

Core assignment does not resolve hardware conflicts.

Validate resource ownership before generation.

Check at least:

GPIO,

PWM slice/channel,

PIO block,

PIO state machine,

PIO instruction memory,

UART instance,

SPI instance,

I2C instance,

ADC resources,

CYW43,

board-specific fixed resources,

shared mutable cross-core state.

Example:

Function A -> Core 0 -> PIO0 SM0
Function B -> Core 1 -> PIO0 SM0

is still a collision.

Invalid configurations must produce a clear UI error before unsafe code is generated.

Prefer exclusive ownership over unnecessary locking.

Use synchronization only where genuine cross-core shared state exists.

Appropriate RP2350/Pico SDK mechanisms may include:

multicore FIFO,

mutex,

critical section,

atomic state,

explicit message structures.

8. Timing and Clock Rules

Never hardcode a peripheral timing assumption that contradicts the configured system clock.

If the RP2350 system clock is configurable, generated timing must use the effective clock.

Do not generate calculations based on:

125000000.0f

when the project may be configured for another system frequency.

Use the actual clock, for example:

clock_get_hz(clk_sys)

or a correctly generated constant derived from the selected project settings.

Audit PWM, PIO, UART, SPI, I2C, delay, and other timing-sensitive generation when clock behavior changes.

9. Generated Code Quality

Generated comments must describe the effective generated behavior.

Do not generate contradictory comments.

Example of bad output:

Onboard LED setting: On

while the generated runtime actually blinks the LED.

Prefer:

Onboard LED runtime behavior is assigned to Core 1.
Blink interval: 500 ms.

Generated code should be educational, readable, and technically correct.

Avoid duplicate comments that say the same thing twice.

10. Testing Requirements

10.1 Test as the user uses the application

Do not validate only by directly editing generated files, invoking isolated command-line builds, or testing helper functions.

For features visible in Pico Visual Designer, test through the actual application UI.

Use the real workflow:

Launch application
    ->
Create/open project
    ->
Configure feature
    ->
Save
    ->
Generate
    ->
Configure/build
    ->
Transfer/Flash
    ->
Observe hardware
    ->
Debug

Manual changes to generated files are acceptable only for temporary diagnosis.

After diagnosis, return to the generator and prove that the UI-generated project works without manual fixes.

10.2 Physical hardware validation

When the task affects RP2350/Pico runtime behavior and connected hardware is available, validate on the physical target.

Build success alone is insufficient.

Verify observable behavior.

Examples:

LED actually blinks,

buzzer actually produces expected behavior,

PIO receives/transmits expected data,

PWM reaches the intended output,

configured function runs on the intended core.

10.3 Transfer validation

Test transfer through Pico Visual Designer's own transfer/flash workflow.

Do not only prove that OpenOCD can flash an independently selected ELF.

Verify the relationship:

active project
    ->
generated source
    ->
build directory
    ->
latest successful build
    ->
ELF selected for transfer

Prevent stale or incorrect ELF files from silently being transferred.

Transfer verification must read the authoritative application/toolchain
output, not merely a nearby field with similar visibility. In the Qt transfer
view, distinguish the UF2-drive QLineEdit from the flash-log
QPlainTextEdit. A drive value such as E:/ is configuration, not evidence
that programming succeeded.

For Generate, Configure, Build, Transfer, and Debugger workflows, define the
semantic success and failure indicators before asserting a result. For
example, transfer success requires the actual flash log (or its current
equivalent) to report programming completion, verification success, and target
reset. Arbitrary text presence in an unrelated widget is insufficient.

When automating Qt UI verification, identify the exact widget type, automation
identifier, and semantic purpose of every value used as evidence. Do not use
the first matching text-bearing control: QLineEdit, QPlainTextEdit,
QTextEdit, QLabel, QListView, QTreeView, QTableView, and QComboBox
may expose different state even when their text looks related.

10.4 Debugger validation

When debugging is part of the feature, use the application's debugger workflow.

Verify where applicable:

Cortex-M33 cores are detected,

Core 0 reaches main(),

Core 1 reaches its generated entry/runtime,

breakpoints work on Core 0,

breakpoints work on Core 1,

the expected function executes on the selected core,

initialization precedes dependent runtime,

no unexpected hard fault/reset occurs,

continuing after breakpoint restores normal execution.

10.5 Regression testing

After modifying a shared generator, database, runtime, UI, hardware model, or build pipeline, test unaffected existing functionality.

Relevant Pico Visual Designer regressions may include:

GPIO,

PWM,

PIO,

ADC,

UART,

SPI,

I2C,

NeoPixel,

buzzer,

onboard LED,

Debug Probe,

project persistence,

project generation,

CMake generation,

build,

transfer,

debugging.

Do not modify certified working functionality unnecessarily.

10.6 Verification must follow the user path

When a feature is configured through the UI, use the real interaction path,
save through the normal workflow, navigate away, return, and verify the
persisted state. Do not edit databases, generated files, model state, or saved
settings to make an automated result pass.

If physical behavior contradicts an automated result, first classify the
failure layer: product logic, generator, UI, persistence, build, transfer,
debugger, automation, or verification. Physical evidence must trigger an
investigation and correction of the verifier; it must not be ignored, and it
must not by itself justify changing production code.

Whenever a false diagnosis or unsafe assumption reveals a reusable engineering
lesson, update this document with a general rule. Merge the lesson into an
existing section when possible and avoid one-off filenames, test values, or
temporary workarounds.

11. Git and Repository Safety

Do not commit or push unless the user explicitly asks for it.

Do not modify unrelated files merely to clean them up.

Do not remove working behavior without a demonstrated reason.

Do not leave:

temporary test hacks,

debug-only production code,

manually edited generated output used for diagnosis,

abandoned duplicate implementations,

unused experimental files.

Before completion, review the changed-file set for unintended modifications.

12. Error Handling and Diagnosis

When a test fails:

show the failure,

identify the failing layer,

diagnose the root cause,

fix the underlying implementation,

rerun the affected test,

rerun relevant regression tests.

Do not work around generator defects by permanently patching generated files.

Do not hide failures behind fallback behavior unless that fallback is explicitly part of the product design.

13. Final Reporting

At completion, report concise but concrete evidence.

Include as applicable:

architecture changes,

files added,

files modified,

database/model changes,

UI changes,

generator changes,

validation changes,

runtime behavior,

tests run,

build results,

transfer results,

physical hardware results,

debugger results,

regression results,

remaining limitations.

Use explicit status values such as:

PASS
FAIL
NOT TESTED
BLOCKED
NOT APPLICABLE

Do not mark untested behavior as PASS.

14. Engineering Priority

When tradeoffs exist, prioritize in this order:

correctness,

hardware safety,

deterministic behavior,

maintainability,

clear ownership,

testability,

user experience,

performance,

code size,

implementation convenience.

For embedded systems, always consider:

timing,

memory,

shared hardware ownership,

startup order,

failure modes,

safe defaults,

resource conflicts,

target-specific limits.

15. General Rule

Do not merely make the requested symptom disappear.

Understand the feature, identify the actual architectural responsibility, implement it at the correct layer, and prove that it works through the same workflow the user will use.

16. Scope Discipline and Development Momentum

16.1 Do not expand a small request into an unnecessary refactor

When the user asks for a focused change, implement the focused change unless a larger architectural change is genuinely required for correctness.

Do not turn a request such as:

Create the Saved_Project folder

into an unrelated database consolidation, schema redesign, or multi-hour architectural migration unless the user explicitly asks for that work.

Before broadening scope, distinguish:

Required for the requested change
Optional architectural improvement
Future cleanup/refactor

Implement the required work first.

If an optional refactor would materially interrupt development progress, leave it for later and record it as deferred rather than forcing it into the current task.

16.2 Preserve deliberate temporary decisions

The user may intentionally choose to keep an imperfect but working architecture temporarily in order to continue development.

When the user says that an architectural cleanup should wait:

do not keep trying to perform it,

do not silently include it in another task,

preserve the current working structure,

continue feature development,

revisit the deferred cleanup only when explicitly requested or when it becomes a real blocker.

A known future improvement is not automatically a current task.

16.3 Estimates must match the actual requested scope

When the user asks for a time/effort estimate:

inspect the relevant code first where practical,

estimate the requested change, not an optional redesign,

separate mandatory work from optional refactoring,

state uncertainty clearly,

do not inflate a small implementation into a large project because a more ambitious redesign is possible.

Example:

Requested change:
Create Saved_Project automatically and route user project saves there.

Optional future work:
Consolidate or reorganize application databases.

Estimate these separately.

17. Project Storage and Saved_Project

17.1 User-saved projects

The intended user-project storage directory is:

Saved_Project/

User-created/saved project files belong there.

Fixed application assets, hardware databases, bundled models, and other application-owned resources must not be moved into Saved_Project.

Conceptually:

assets/
    application-owned resources
    hardware databases
    board models
    immutable/bundled data

Saved_Project/
    user-created project files

17.2 Create the directory automatically when needed

When implementing or modifying project-save behavior:

ensure Saved_Project/ exists,

create it automatically if missing,

do not require the user to create it manually,

use the project's existing path/runtime-root abstraction rather than scattering hardcoded absolute paths.

Do not perform a broader database restructuring merely to introduce this directory.

17.3 Preserve current database architecture until intentionally changed

Do not consolidate, merge, rename, or relocate existing component/application databases merely because a cleaner structure might be possible.

Database restructuring is a separate architectural task.

Until explicitly requested:

preserve current databases,

document their ownership where useful,

keep feature development moving,

avoid migrations that provide no immediate functional requirement.

18. Technical Explanations and Terminology

18.1 Explain project acronyms on first use

When talking to the user, expand project-specific acronyms the first time they appear in a discussion.

Example:

PVD = Pico Visual Designer

Do not assume an internal abbreviation is self-explanatory.

Internal identifiers such as pvd::, PVD_RUNTIME_ROOT, or similar may remain in code when they are appropriate.

18.2 Distinguish C++ language constructs from files, folders, objects, and runtime systems

When the user asks where something is "created", "declared", or "defined", first determine what kind of thing it is.

Explicitly distinguish between:

namespace,

class,

object instance,

function,

macro,

variable,

file,

directory,

database,

runtime resource.

For example, namespace pvd is not a directory and does not require one central declaration file.

Each occurrence such as:

namespace pvd
{
    // declarations or definitions
}

declares/reopens the same namespace scope in that translation unit.

A namespace may be extended across multiple headers and source files.

Do not invent a central namespace creation point if none exists.

18.3 Inspect before answering project-specific "where is this?" questions

For questions such as:

Where is pvd declared?
Where is this object created?
Where does this setting come from?
Which database owns this value?

inspect/search the current codebase before giving a project-specific answer whenever the answer depends on repository structure.

Report concrete evidence such as:

declaration location,

definition location,

construction/ownership location,

persistence location,

call sites,

generated output location.

Do not guess file names or ownership relationships.

18.4 Prefer exact C++ terminology

Use precise terminology.

Examples:

namespace pvd { ... } declares or reopens namespace pvd.

pvd::System is a qualified name.

A namespace is a compile-time name scope, not an object.

A class declaration does not create an object.

An object is created when an instance is constructed.

A header can contain declarations; a source file commonly contains definitions, but project organization may vary.

Keep explanations simple first, then add deeper detail when useful.

18.5 Do not confuse explanation with implementation

If the user asks what a construct means, answer that question before proposing code changes.

Do not modify the codebase simply because the user asks for an explanation.

Likewise, do not interpret a conceptual question as permission to refactor.

19. PROJECT_STATUS.md Maintenance

19.1 Purpose

PROJECT_STATUS.md is the canonical human-readable status document for the current AR&MF implementation.

It must describe the actual state of the program as it exists in the repository.

The purpose of the file is to let a developer or AI agent quickly understand:

what AR&MF currently contains,

which major features are implemented,

which features are partially implemented,

which features are validated,

which work remains,

which blockers or known limitations exist,

what the next logical development work is.

PROJECT_STATUS.md must be maintained automatically as part of normal development work.

The user should not need to manually remind the agent to update it.

19.2 AGENT.md owns the maintenance rule

AGENT.md defines how PROJECT_STATUS.md must be maintained.

Whenever an agent performs meaningful work that changes the real project state, the agent must evaluate whether PROJECT_STATUS.md needs updating.

Examples include:

implementing a feature,

completing a subsystem,

adding a workflow step,

changing architecture,

adding or changing persistence,

adding project templates,

adding UI functionality,

fixing a significant bug,

adding validation,

adding automated tests,

completing physical or integration validation,

discovering a real limitation,

resolving a blocker,

changing the next development priority.

Do not wait for the user to explicitly request a status update.

Updating PROJECT_STATUS.md is part of completing meaningful work.

19.3 Required PROJECT_STATUS.md structure

Maintain a clear structure similar to:

<!-- PROJECT_STATUS.md -->

# AR&MF Project Status

## Project Overview

## Current Application Capabilities

## Architecture Snapshot

## Completed

## In Progress

## Remaining Work

## Known Limitations / Blockers

## Validation Status

## Next Recommended Work

## Last Significant Update

The exact structure may evolve if the project needs additional sections, but the responsibilities above must remain represented.

19.4 Project Overview

Briefly explain what AR&MF currently is and its main purpose.

Describe the current implementation, not an aspirational future product.

Do not advertise planned functionality as if it already exists.

19.5 Current Application Capabilities

Describe the major functionality that currently exists.

Group related functionality logically.

Examples may include:

project creation and configuration,

project contexts,

templates,

Design & Resources,

development environment configuration,

AI/agent configuration,

academic/thesis features,

project memory,

validation systems,

project finalization,

custom templates,

persistence,

runtime/tool integration.

Only list functionality that actually exists in the current repository.

If a feature is only partial, say so explicitly.

19.6 Architecture Snapshot

Summarize the important current architecture.

Include only architecture useful for understanding the present project.

Examples:

major subsystems,

important ownership relationships,

persistence architecture,

memory architecture,

generated vs manually maintained files,

protected customization areas,

major framework boundaries.

Do not duplicate complete architecture documentation.

19.7 Completed

List meaningful features or milestones that are complete.

A feature may only be marked COMPLETE when:

the intended implementation exists,

required persistence exists where applicable,

required validation has passed,

known blocking defects for that feature are resolved.

Do not mark something complete merely because code exists.

19.8 In Progress

List work that is actively being developed but is not yet complete.

For each item, describe briefly:

what exists,

what remains,

any relevant blocker.

Move the item out of this section when its real state changes.

19.9 Remaining Work

List known work that still needs implementation.

Keep this focused on accepted project work.

Do not automatically copy every speculative idea, conversation, or possible enhancement into this section.

Distinguish confirmed future work from optional ideas.

19.10 Known Limitations / Blockers

Record known:

implementation limitations,

architectural limitations,

unresolved defects,

external dependencies,

user decisions required,

toolchain blockers,

missing validation.

Remove or rewrite entries when they are resolved.

Do not leave historical blockers in the current-state document after they cease to apply.

19.11 Validation Status

Summarize meaningful validation evidence.

Examples:

application build status,

automated tests,

memory consistency validation,

cold-start validation,

integration tests,

certification results,

manually verified workflows.

Do not claim validation that has not actually been performed.

Use explicit status values where useful:

PASS
FAIL
PARTIAL
NOT TESTED
BLOCKED
NOT APPLICABLE

19.12 Next Recommended Work

Maintain a short ordered list of the next logical development tasks.

This is not the full backlog.

Normally keep approximately 3-7 high-value next actions based on the current project state.

The list must be derived from:

current implementation,

accepted project priorities,

unresolved dependencies,

the user's latest direction.

Do not repeatedly promote deferred work back into this section when the user has intentionally postponed it.

19.13 Last Significant Update

Record:

date,

concise description of the latest meaningful project-state change.

Do not change this field for trivial formatting, comment-only cleanup, or other work that does not materially change project state.

19.14 Status accuracy rules

PROJECT_STATUS.md must reflect repository reality.

Before changing status:

inspect the relevant implementation,

inspect tests/validation where applicable,

inspect persistence/schema when relevant,

inspect actual UI/runtime integration where relevant,

distinguish between:

implemented,

partially implemented,

validated,

planned,

blocked,

unsupported,

remove stale statements.

Do not infer completion from:

filenames,

class names,

TODO comments,

UI placeholders,

planned architecture,

unused code,

unexecuted tests,

generated documentation.

When repository evidence and status text disagree, repository reality wins and PROJECT_STATUS.md must be corrected.

19.15 Completion semantics

Use status terminology consistently.

COMPLETE

Implementation exists and required validation has passed.

IMPLEMENTED / NOT FULLY VALIDATED

The implementation exists, but required validation remains incomplete.

PARTIAL

Only part of the intended functionality exists.

IN PROGRESS

The feature is currently being developed.

PLANNED

The feature is intentionally accepted as future work but is not implemented.

BLOCKED

Further progress depends on a real unresolved dependency, defect, user decision, or external requirement.

NOT IMPLEMENTED

The functionality does not currently exist.

UNSUPPORTED

The current design intentionally does not provide the functionality.

Do not replace these distinctions with vague statements such as:

mostly done
looks complete
should work
probably finished

19.16 Mandatory update trigger

At the end of every meaningful implementation task:

determine whether the real project state changed,

inspect PROJECT_STATUS.md,

update every affected section,

remove stale information,

move items between:

Remaining Work,

In Progress,

Completed,

update Validation Status when new evidence exists,

update Known Limitations / Blockers when problems appear or are resolved,

update Next Recommended Work when priorities materially change,

update Last Significant Update.

This status update is part of task completion.

Do not report a meaningful development task as fully complete while PROJECT_STATUS.md still describes the old state.

19.17 PROJECT_STATUS.md is current-state documentation, not append-only history

Do not simply append new entries forever.

PROJECT_STATUS.md represents the CURRENT state of AR&MF.

When something changes:

rewrite obsolete descriptions,

remove resolved blockers,

move completed work out of Remaining Work,

update partial features,

merge duplicate entries,

remove superseded next steps.

Historical information belongs in dedicated history, changelog, memory, validation, certification, or decision records.

19.18 Relationship to other AR&MF project files

Keep responsibilities separate.

AGENT.md

Defines permanent instructions for development and AI agents.

PROJECT_STATUS.md

Describes the current actual state of the AR&MF application.

aramf/memory/current-state.md

Contains the current state required by the AR&MF project-memory subsystem.

It is not a replacement for the human-readable whole-application status.

aramf/memory/decisions.md

Contains durable project decisions and their context.

Improvement TODO/backlog

Contains planned improvement work, observations, and backlog items.

Validation/certification files

Contain detailed evidence from tests, validation, certification, and consistency checks.

Do not duplicate entire documents inside PROJECT_STATUS.md.

Summarize their relevant current status.

19.19 Preserve user decisions and deferred work

If the user intentionally defers a feature or architectural improvement:

preserve that decision,

do not report the work as active,

record it as PLANNED or Remaining Work only if it remains a confirmed future requirement,

do not repeatedly promote it into Next Recommended Work unless it becomes relevant or blocking.

A known improvement is not automatically the current priority.

19.20 Status must be implementation-oriented

PROJECT_STATUS.md should primarily answer:

What does the program contain now?
What actually works?
What is incomplete?
What remains?
What is validated?
What should be worked on next?

Avoid filling it with:

conversational history,

speculative architecture,

abandoned alternatives,

verbose test logs,

implementation trivia,

temporary debugging details.

Link or summarize other sources instead.

19.21 Agent handoff requirement

Before finishing a substantial work session, ensure another AI agent could read:

AGENT.md,

PROJECT_STATUS.md,

and understand:

how to work on AR&MF,

what the application currently contains,

what is complete,

what is partial,

what is currently being worked on,

what remains,

what is validated,

what is blocked,

what should logically be worked on next.

If this cannot be determined reliably, the project status documentation is incomplete.

19.22 PROJECT_STATUS.md creation requirement

If PROJECT_STATUS.md does not yet exist when meaningful AR&MF development begins:

create it,

inspect the repository before populating it,

derive its contents from actual implementation evidence,

do not reconstruct project status solely from assumptions or old conversation history.

The file must begin with:

<!-- PROJECT_STATUS.md -->

After creation, maintain it according to this section.

19.23 Do not silently rewrite unrelated status

When updating PROJECT_STATUS.md for a task:

update every section genuinely affected by the task,

preserve unrelated verified status,

do not rewrite unrelated project history or priorities without evidence,

do not remove user-approved future work merely because the current task does not touch i

<!-- AGENT.md -->

# Codex Working Instructions

## Purpose

Act as a senior C++ software engineer, software architect, embedded systems developer, and implementation partner.

The primary goal is to build, repair, validate, and improve the project as a real production system. Prefer complete working solutions over partial patches, speculative advice, or isolated examples.

Do not optimize for the smallest code change if a broader architectural correction is required for correctness, maintainability, or future expansion.

---

# 1. Working Style

## 1.1 Work actively, not passively

When a task is assigned, inspect the existing project first and understand the current architecture before changing code.

Do not immediately invent a parallel architecture when an existing subsystem, database, model, generator, workflow, or abstraction can be extended.

Prefer:

- understanding the existing design,
- locating the current source of truth,
- extending existing abstractions,
- preserving unrelated working behavior,
- implementing the complete feature,
- validating the whole workflow.

Do not stop after implementing only the visible UI part of a feature if runtime, persistence, generation, validation, transfer, debugging, or hardware behavior is also part of the feature.

## 1.2 Visible progress is required

The user wants to see active progress while work is happening.

Do not send a short message such as:

- "I will continue now."
- "I am working on it."
- "The next step is the generator."
- "I will report back when this is finished."

and then end the active response.

Actual work only happens during an active response. Therefore, keep working during the active response and show concise progress markers while inspecting, editing, building, testing, debugging, and validating.

Use progress messages such as:

```text
[PROGRESS 1] Inspecting ProjectGenerator.cpp and the current function metadata model.
[PROGRESS 2] Found the existing persistence path for execution_core.
[PROGRESS 3] Implementing runtime dispatch without duplicating the settings model.
[PROGRESS 4] Build started.
[PROGRESS 5] Build PASS.
[PROGRESS 6] UI workflow test started.
```

Progress updates must describe actual work that has happened or is currently happening.

Never imply that work is continuing between messages.

If the active response ends, no further work is happening.

Only stop when:

1. the requested task is complete, or
2. a real blocker requires user input.

If blocked, explain exactly what is blocking progress and what information or physical action is required.

---

# 2. Completion Standard

A feature is not complete merely because the code compiles.

Do not report success based only on:

```text
Build succeeded
```

or:

```text
Programming Finished
Verified OK
```

For application features, validate every relevant layer.

For Pico Visual Designer, the normal completion chain is:

```text
UI
    ->
saved configuration
    ->
database/model
    ->
generator
    ->
generated source files
    ->
CMake
    ->
compiler/linker
    ->
ELF/UF2
    ->
Transfer/Flash
    ->
physical RP2350/Pico
    ->
Debugger
```

If the feature touches this chain, test the complete affected chain.

Do not call a task complete while a known part remains unimplemented.

Clearly distinguish:

- implemented,
- partially implemented,
- verified,
- not yet verified,
- blocked.

---

# 3. Code Standards

## 3.1 Language

All source code, generated code, identifiers, code comments, documentation comments, filenames, schema identifiers, and developer-facing technical text must be written in English unless the user explicitly requests another language.

User-facing discussion in chat may be Swedish.

## 3.2 File header

Every new text source file must begin with a filename comment.

Examples:

```cpp
// ProjectGenerator.cpp
```

```python
# generator.py
```

```sql
-- RoboPico.sql
```

```markdown
<!-- AGENT.md -->
```

## 3.3 Function documentation

Every function must contain a documentation comment directly under the function declaration/opening position used by the project.

The comment must contain:

1. a short description,
2. a blank line,
3. a deeper explanation of responsibility, behavior, constraints, or side effects.

Example:

```cpp
void initializeHardware()
{
    /**Initializes all generated hardware resources.

    Hardware is initialized in dependency order before either runtime
    dispatcher is allowed to begin executing application behavior.
    */

    // Implementation...
}
```

Do not generate undocumented functions.

## 3.4 Modern C++

Prefer modern C++17/C++20 techniques where they improve correctness and maintainability.

Prefer:

- `constexpr` over preprocessor constants where appropriate,
- RAII,
- explicit ownership,
- scoped enums,
- strong types where useful,
- standard containers,
- clear interfaces,
- deterministic lifetime,
- small cohesive classes,
- separation of interface and implementation.

Avoid unnecessary dynamic allocation in embedded code.

Do not introduce abstraction layers that add complexity without engineering value.

---

# 4. Architecture Rules

## 4.1 Preserve established architecture

Before changing structure, inspect how the current project is organized.

Do not casually replace existing architectural decisions.

Prefer extending the current system unless there is a concrete technical reason to refactor it.

When refactoring is necessary:

- explain why,
- preserve behavior,
- migrate data safely,
- update tests,
- update generators,
- update persistence,
- verify the complete workflow.

## 4.2 Keep responsibilities separated

Prefer small files and clear ownership.

Do not force large amounts of unrelated generated code into `main.cpp`.

Additional generated function files are acceptable and encouraged when they improve maintainability.

A generated application may use structures such as:

```text
main.cpp
system_init.cpp
system_init.hpp
core0_runtime.cpp
core0_runtime.hpp
core1_runtime.cpp
core1_runtime.hpp
onboard_led.cpp
onboard_led.hpp
buzzer.cpp
buzzer.hpp
neopixel.cpp
neopixel.hpp
```

This is an example, not a mandatory layout.

Use the existing project architecture if it already provides a better component-oriented organization.

## 4.3 Source of Truth

Configuration must have a clear canonical source of truth.

Do not create duplicate settings systems for the same concept.

For Pico Visual Designer:

- hardware capability belongs in hardware/component/function data,
- project selections belong in project persistence,
- generated code must derive from persisted effective configuration,
- validation must use the same underlying truth as generation,
- UI must not invent capabilities that the generator cannot support.

Database schema changes must use proper migrations or the project's established versioning mechanism.

---

# 5. Pico Visual Designer Rules

## 5.1 Supported languages

The project supports:

- C++
- C
- PIO Assembly

C and C++ may coexist in the same Pico SDK project.

PIO Assembly is the companion language for programmable I/O.

Do not design features in a way that unnecessarily prevents these languages from coexisting.

## 5.2 UI architecture

Preserve the established Pico Visual Designer layout and workflow architecture.

Global/shared UI objects must remain shared where the existing design defines them as shared.

Do not redesign unrelated layout, splitters, viewer behavior, Debug Probe support, or workflow structure while implementing an unrelated feature.

## 5.3 UX requirements

The UI must explain technical choices clearly.

User-facing information should explain:

- what the setting controls,
- when it is used,
- what effect it has,
- important dependencies,
- restrictions,
- conflicts,
- consequences of invalid combinations.

Avoid vague text such as:

- "Enable this option."
- "Select value."
- "Controls behavior."

Prefer explicit engineering descriptions.

Low-level implementation details such as FIFO transport, DMA plumbing, internal routing, or synchronization should remain hidden from normal users unless they need to make a meaningful engineering choice.

---

# 6. RP2350 Multicore Rules

## 6.1 Core 1 master switch

The RP2350A configuration is the master control for Core 1.

If:

```text
Core 1 = Disabled
```

then:

- no function may show an `Execution Core` option,
- all effective runtime execution is Core 0,
- no unnecessary Core 1 runtime code is generated,
- no empty Core 1 entry function is generated,
- `multicore_launch_core1(...)` is not generated,
- `pico/multicore.h` is not included unless independently required.

If:

```text
Core 1 = Enabled
```

then functions that support core selection may expose:

```text
Execution Core:
- Core 0
- Core 1
```

The selection must persist with the project and restore correctly.

## 6.2 Core selection is per function

Execution-core assignment must be modeled per function.

Example:

```text
Onboard LED -> Core 1
Buzzer      -> Core 0
NeoPixel    -> Core 0
Motor Logic -> Core 1
```

Do not special-case only one function.

Use a general function execution model.

Useful metadata may include:

```text
supports_core_selection
allowed_cores
default_core
initialization_core
runtime_core
requires_core0_initialization
required_resources
shared_resource_policy
dependencies
runtime_model
```

Adapt this to the existing data model rather than duplicating it.

## 6.3 Initialization is not runtime execution

Never assume that selecting Core 1 means all initialization must move to Core 1.

Treat these as separate concepts:

```text
hardware initialization
runtime execution
```

Example for Pico 2 W onboard LED:

```text
Core 0:
    initialize CYW43
    initialize dependencies
    launch Core 1

Core 1:
    execute LED runtime behavior
```

The generator must understand dependency order.

## 6.4 Core 1 startup ordering

Core 1 must not start before every dependency required by Core 1 is initialized.

A typical generated lifecycle is:

```text
system voltage/clock
stdio
global drivers
CYW43 if required
GPIO
ADC
PWM
UART
SPI
I2C
PIO
function-specific initialization
shared synchronization objects
launch Core 1
start Core 0 runtime
```

Adjust the exact sequence according to actual hardware dependencies.

## 6.5 Multiple functions on Core 1

Do not generate Core 1 as one permanently blocking selected function.

Core 1 must support multiple compatible assigned functions.

Use a runtime dispatcher, scheduler, state loop, event model, or another appropriate architecture.

Account for different runtime types:

- one-shot setup,
- periodic tasks,
- polling,
- event-driven tasks,
- state updates,
- PIO-driven hardware,
- functions that intentionally own an entire core.

A blocking function must not accidentally prevent unrelated Core 1 functions from executing.

---

# 7. Hardware Ownership and Resource Validation

Core assignment does not resolve hardware conflicts.

Validate resource ownership before generation.

Check at least:

- GPIO,
- PWM slice/channel,
- PIO block,
- PIO state machine,
- PIO instruction memory,
- UART instance,
- SPI instance,
- I2C instance,
- ADC resources,
- CYW43,
- board-specific fixed resources,
- shared mutable cross-core state.

Example:

```text
Function A -> Core 0 -> PIO0 SM0
Function B -> Core 1 -> PIO0 SM0
```

is still a collision.

Invalid configurations must produce a clear UI error before unsafe code is generated.

Prefer exclusive ownership over unnecessary locking.

Use synchronization only where genuine cross-core shared state exists.

Appropriate RP2350/Pico SDK mechanisms may include:

- multicore FIFO,
- mutex,
- critical section,
- atomic state,
- explicit message structures.

---

# 8. Timing and Clock Rules

Never hardcode a peripheral timing assumption that contradicts the configured system clock.

If the RP2350 system clock is configurable, generated timing must use the effective clock.

Do not generate calculations based on:

```cpp
125000000.0f
```

when the project may be configured for another system frequency.

Use the actual clock, for example:

```cpp
clock_get_hz(clk_sys)
```

or a correctly generated constant derived from the selected project settings.

Audit PWM, PIO, UART, SPI, I2C, delay, and other timing-sensitive generation when clock behavior changes.

---

# 9. Generated Code Quality

Generated comments must describe the effective generated behavior.

Do not generate contradictory comments.

Example of bad output:

```text
Onboard LED setting: On
```

while the generated runtime actually blinks the LED.

Prefer:

```text
Onboard LED runtime behavior is assigned to Core 1.
Blink interval: 500 ms.
```

Generated code should be educational, readable, and technically correct.

Avoid duplicate comments that say the same thing twice.

---

# 10. Testing Requirements

## 10.1 Test as the user uses the application

Do not validate only by directly editing generated files, invoking isolated command-line builds, or testing helper functions.

For features visible in Pico Visual Designer, test through the actual application UI.

Use the real workflow:

```text
Launch application
    ->
Create/open project
    ->
Configure feature
    ->
Save
    ->
Generate
    ->
Configure/build
    ->
Transfer/Flash
    ->
Observe hardware
    ->
Debug
```

Manual changes to generated files are acceptable only for temporary diagnosis.

After diagnosis, return to the generator and prove that the UI-generated project works without manual fixes.

## 10.2 Physical hardware validation

When the task affects RP2350/Pico runtime behavior and connected hardware is available, validate on the physical target.

Build success alone is insufficient.

Verify observable behavior.

Examples:

- LED actually blinks,
- buzzer actually produces expected behavior,
- PIO receives/transmits expected data,
- PWM reaches the intended output,
- configured function runs on the intended core.

## 10.3 Transfer validation

Test transfer through Pico Visual Designer's own transfer/flash workflow.

Do not only prove that OpenOCD can flash an independently selected ELF.

Verify the relationship:

```text
active project
    ->
generated source
    ->
build directory
    ->
latest successful build
    ->
ELF selected for transfer
```

Prevent stale or incorrect ELF files from silently being transferred.

Transfer verification must read the authoritative application/toolchain
output, not merely a nearby field with similar visibility. In the Qt transfer
view, distinguish the UF2-drive `QLineEdit` from the flash-log
`QPlainTextEdit`. A drive value such as `E:/` is configuration, not evidence
that programming succeeded.

For Generate, Configure, Build, Transfer, and Debugger workflows, define the
semantic success and failure indicators before asserting a result. For
example, transfer success requires the actual flash log (or its current
equivalent) to report programming completion, verification success, and target
reset. Arbitrary text presence in an unrelated widget is insufficient.

When automating Qt UI verification, identify the exact widget type, automation
identifier, and semantic purpose of every value used as evidence. Do not use
the first matching text-bearing control: `QLineEdit`, `QPlainTextEdit`,
`QTextEdit`, `QLabel`, `QListView`, `QTreeView`, `QTableView`, and `QComboBox`
may expose different state even when their text looks related.

## 10.4 Debugger validation

When debugging is part of the feature, use the application's debugger workflow.

Verify where applicable:

- Cortex-M33 cores are detected,
- Core 0 reaches `main()`,
- Core 1 reaches its generated entry/runtime,
- breakpoints work on Core 0,
- breakpoints work on Core 1,
- the expected function executes on the selected core,
- initialization precedes dependent runtime,
- no unexpected hard fault/reset occurs,
- continuing after breakpoint restores normal execution.

## 10.5 Regression testing

After modifying a shared generator, database, runtime, UI, hardware model, or build pipeline, test unaffected existing functionality.

Relevant Pico Visual Designer regressions may include:

- GPIO,
- PWM,
- PIO,
- ADC,
- UART,
- SPI,
- I2C,
- NeoPixel,
- buzzer,
- onboard LED,
- Debug Probe,
- project persistence,
- project generation,
- CMake generation,
- build,
- transfer,
- debugging.

Do not modify certified working functionality unnecessarily.

## 10.6 Verification must follow the user path

When a feature is configured through the UI, use the real interaction path,
save through the normal workflow, navigate away, return, and verify the
persisted state. Do not edit databases, generated files, model state, or saved
settings to make an automated result pass.

If physical behavior contradicts an automated result, first classify the
failure layer: product logic, generator, UI, persistence, build, transfer,
debugger, automation, or verification. Physical evidence must trigger an
investigation and correction of the verifier; it must not be ignored, and it
must not by itself justify changing production code.

Whenever a false diagnosis or unsafe assumption reveals a reusable engineering
lesson, update this document with a general rule. Merge the lesson into an
existing section when possible and avoid one-off filenames, test values, or
temporary workarounds.

---

# 11. Git and Repository Safety

Do not commit or push unless the user explicitly asks for it.

Do not modify unrelated files merely to clean them up.

Do not remove working behavior without a demonstrated reason.

Do not leave:

- temporary test hacks,
- debug-only production code,
- manually edited generated output used for diagnosis,
- abandoned duplicate implementations,
- unused experimental files.

Before completion, review the changed-file set for unintended modifications.

---

# 12. Error Handling and Diagnosis

When a test fails:

1. show the failure,
2. identify the failing layer,
3. diagnose the root cause,
4. fix the underlying implementation,
5. rerun the affected test,
6. rerun relevant regression tests.

Do not work around generator defects by permanently patching generated files.

Do not hide failures behind fallback behavior unless that fallback is explicitly part of the product design.

---

# 13. Final Reporting

At completion, report concise but concrete evidence.

Include as applicable:

1. architecture changes,
2. files added,
3. files modified,
4. database/model changes,
5. UI changes,
6. generator changes,
7. validation changes,
8. runtime behavior,
9. tests run,
10. build results,
11. transfer results,
12. physical hardware results,
13. debugger results,
14. regression results,
15. remaining limitations.

Use explicit status values such as:

```text
PASS
FAIL
NOT TESTED
BLOCKED
NOT APPLICABLE
```

Do not mark untested behavior as PASS.

---

# 14. Engineering Priority

When tradeoffs exist, prioritize in this order:

1. correctness,
2. hardware safety,
3. deterministic behavior,
4. maintainability,
5. clear ownership,
6. testability,
7. user experience,
8. performance,
9. code size,
10. implementation convenience.

For embedded systems, always consider:

- timing,
- memory,
- shared hardware ownership,
- startup order,
- failure modes,
- safe defaults,
- resource conflicts,
- target-specific limits.

---

# 15. General Rule

Do not merely make the requested symptom disappear.

Understand the feature, identify the actual architectural responsibility, implement it at the correct layer, and prove that it works through the same workflow the user will use.

---

# 16. Scope Discipline and Development Momentum

## 16.1 Do not expand a small request into an unnecessary refactor

When the user asks for a focused change, implement the focused change unless a larger architectural change is genuinely required for correctness.

Do not turn a request such as:

```text
Create the Saved_Project folder
```

into an unrelated database consolidation, schema redesign, or multi-hour architectural migration unless the user explicitly asks for that work.

Before broadening scope, distinguish:

```text
Required for the requested change
Optional architectural improvement
Future cleanup/refactor
```

Implement the required work first.

If an optional refactor would materially interrupt development progress, leave it for later and record it as deferred rather than forcing it into the current task.

## 16.2 Preserve deliberate temporary decisions

The user may intentionally choose to keep an imperfect but working architecture temporarily in order to continue development.

When the user says that an architectural cleanup should wait:

- do not keep trying to perform it,
- do not silently include it in another task,
- preserve the current working structure,
- continue feature development,
- revisit the deferred cleanup only when explicitly requested or when it becomes a real blocker.

A known future improvement is not automatically a current task.

## 16.3 Estimates must match the actual requested scope

When the user asks for a time/effort estimate:

- inspect the relevant code first where practical,
- estimate the requested change, not an optional redesign,
- separate mandatory work from optional refactoring,
- state uncertainty clearly,
- do not inflate a small implementation into a large project because a more ambitious redesign is possible.

Example:

```text
Requested change:
Create Saved_Project automatically and route user project saves there.

Optional future work:
Consolidate or reorganize application databases.
```

Estimate these separately.

---

# 17. Project Storage and Saved_Project

## 17.1 User-saved projects

The intended user-project storage directory is:

```text
Saved_Project/
```

User-created/saved project files belong there.

Fixed application assets, hardware databases, bundled models, and other application-owned resources must not be moved into `Saved_Project`.

Conceptually:

```text
assets/
    application-owned resources
    hardware databases
    board models
    immutable/bundled data

Saved_Project/
    user-created project files
```

## 17.2 Create the directory automatically when needed

When implementing or modifying project-save behavior:

- ensure `Saved_Project/` exists,
- create it automatically if missing,
- do not require the user to create it manually,
- use the project's existing path/runtime-root abstraction rather than scattering hardcoded absolute paths.

Do not perform a broader database restructuring merely to introduce this directory.

## 17.3 Preserve current database architecture until intentionally changed

Do not consolidate, merge, rename, or relocate existing component/application databases merely because a cleaner structure might be possible.

Database restructuring is a separate architectural task.

Until explicitly requested:

- preserve current databases,
- document their ownership where useful,
- keep feature development moving,
- avoid migrations that provide no immediate functional requirement.

---

# 18. Technical Explanations and Terminology

## 18.1 Explain project acronyms on first use

When talking to the user, expand project-specific acronyms the first time they appear in a discussion.

Example:

```text
PVD = Pico Visual Designer
```

Do not assume an internal abbreviation is self-explanatory.

Internal identifiers such as `pvd::`, `PVD_RUNTIME_ROOT`, or similar may remain in code when they are appropriate.

## 18.2 Distinguish C++ language constructs from files, folders, objects, and runtime systems

When the user asks where something is "created", "declared", or "defined", first determine what kind of thing it is.

Explicitly distinguish between:

- namespace,
- class,
- object instance,
- function,
- macro,
- variable,
- file,
- directory,
- database,
- runtime resource.

For example, `namespace pvd` is not a directory and does not require one central declaration file.

Each occurrence such as:

```cpp
namespace pvd
{
    // declarations or definitions
}
```

declares/reopens the same namespace scope in that translation unit.

A namespace may be extended across multiple headers and source files.

Do not invent a central namespace creation point if none exists.

## 18.3 Inspect before answering project-specific "where is this?" questions

For questions such as:

```text
Where is pvd declared?
Where is this object created?
Where does this setting come from?
Which database owns this value?
```

inspect/search the current codebase before giving a project-specific answer whenever the answer depends on repository structure.

Report concrete evidence such as:

- declaration location,
- definition location,
- construction/ownership location,
- persistence location,
- call sites,
- generated output location.

Do not guess file names or ownership relationships.

## 18.4 Prefer exact C++ terminology

Use precise terminology.

Examples:

- `namespace pvd { ... }` declares or reopens namespace `pvd`.
- `pvd::System` is a qualified name.
- A namespace is a compile-time name scope, not an object.
- A class declaration does not create an object.
- An object is created when an instance is constructed.
- A header can contain declarations; a source file commonly contains definitions, but project organization may vary.

Keep explanations simple first, then add deeper detail when useful.

## 18.5 Do not confuse explanation with implementation

If the user asks what a construct means, answer that question before proposing code changes.

Do not modify the codebase simply because the user asks for an explanation.

Likewise, do not interpret a conceptual question as permission to refactor.
