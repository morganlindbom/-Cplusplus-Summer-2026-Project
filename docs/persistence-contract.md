# PVD persistence contract

The project state boundary is:

`UI -> System -> ApplicationState -> ProjectStore -> SQLite`

UI components do not access project SQLite databases directly. `System` owns the live `ApplicationState`, dirty-state lifecycle, save error handling, and replacement after a successful load.

## ApplicationState classification

| Field | Classification | Persistence / restore mechanism |
|---|---|---|
| `projectName` | PERSISTENT | SQLite `project.project_name` |
| `projectPath` | DERIVED | The directory containing the selected project database |
| `product` | PERSISTENT | SQLite `project.product` |
| `language` | PERSISTENT | SQLite `project.language` |
| `state` | PERSISTENT | SQLite `project.state` |
| `debugSessionTools` | PERSISTENT | SQLite `project.debug_session_tools` |
| `runtimeDiagnostics` | PERSISTENT | SQLite `project.runtime_diagnostics` |
| `verboseBuildEvidence` | PERSISTENT | SQLite `project.verbose_build_evidence` |
| `selections` | PERSISTENT | SQLite `selections` rows |
| `FunctionSelection::settings` | PERSISTENT | SQLite `settings` rows associated with an existing selection |
| `generatedFiles` | DERIVED | Reconstructed from the validated project's `generated/` directory after load; the generator is authoritative for creating those files |
| `System::projectDirty_` | RUNTIME_ONLY | Central `ProjectDirtyState`; never stored in SQLite |
| `System::latestBuildSuccessful_` | RUNTIME_ONLY | Re-established by the current application session/build workflow |
| `System::lastBuiltProjectPath_` | RUNTIME_ONLY | Re-established by the current application session/build workflow |

`generatedFiles` is not treated as authoritative project input. A valid project can reconstruct it from its persistent project location and the generated output directory. The generator remains responsible for producing the directory contents; `ProjectStore::load` only refreshes the derived file list so the reopened UI reflects existing generated artifacts.

## Save contract

`ProjectStore::save` validates the state, creates the project schema if necessary, writes metadata/selections/settings inside one SQLite transaction, and commits only when every required operation succeeds. Any failure rolls back and returns an error.

The `System` controller is the only application save path. A successful save clears dirty state; a failed save keeps dirty state and reports the error through the project status and the existing warning mechanism.

## Load contract

`ProjectStore::load` reads into a temporary `ApplicationState` candidate. Metadata, selections, settings, derived generated files, and invariants are all checked before the destination state is replaced. A failed metadata, selection, settings, or validation step leaves the caller's live state unchanged.

## Dirty contract

Persistent UI mutations update `ApplicationState` and mark the central dirty state. Explicit Save Project performs the transactional save. Successful Create/Open/Save operations clear dirty state; failed save/open operations preserve the relevant previous state. The existing Create/Open workflow asks before discarding unsaved changes.

## Round-trip definition

Round-trip equality applies to the persistent fields only:

```text
PersistentProjectState A -> save -> load -> PersistentProjectState B
A == B
```

Derived fields must satisfy:

```text
generatedFiles_after_load == rebuild_from_project_generated_directory
```

The executable persistence contract tests cover both conditions and real malformed SQLite failure paths.
