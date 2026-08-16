<!-- ARCHITECTURE.md -->
# Architecture contract

## Ownership

- `main.cpp` starts exactly one `System` object.
- `System` is the orchestration root and owns cross-component connections.
- `system.sqlite` is the governing registry. It knows component database locations and workflow routing.
- `MainWindow` is only the permanent visual shell.
- `Workflow` and `Viewer` are single shared objects for the full application lifetime.
- Every workflow owns a unique Column 2 object and a unique Column 3 object.
- Every visible MainWindow component has its own folder, C++ class and local SQLite database.
- Pin/peripheral function knowledge lives in independent function databases; it is not duplicated into `system.sqlite`.

## MainWindow invariant

`Workflow | active workflow Column 2 | active workflow Column 3 | Viewer`

Only the middle two objects change when workflow changes.

## Communication invariant

Cross-component communication is connected by `System`. UI objects emit domain-neutral signals and do not directly reach into sibling components.
