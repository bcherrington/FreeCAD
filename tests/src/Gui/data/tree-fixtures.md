# Tree functional fixtures

These FCStd inputs qualify tree correctness in `TreeBehavior.cpp`. They are not
performance samples or replacements for the spec 003 benchmark fixture set.

| Input | Objects | Structure | SHA-256 |
| --- | --- | --- | --- |
| tree-8.FCStd | 10 | Two nested groups, eight image planes | b664fd8eed579b0d1f1b96dcba204bc8070bfe576b62d8c73040c33f7f34028a |
| tree-512.FCStd | 514 | Two nested groups, 512 image planes | f0849e08d5453e10846fcb6b7c0f67563766ed6f7bfebc08b7ac5febb218d2b5 |

`Assembly` contains `Nested`; `Nested` contains the first half of the planes.
The second half are document roots. Image planes have no image payload, so the
inputs exercise grouping, restore, observers and tree interaction independently
of texture decode and workbench geometry. Both App and Gui document data are
included. Normal tests verify the exact archive hashes before opening them.

The generator is in `TreeBehaviorTest::initTestCase`. To regenerate into a new
scratch directory, explicitly set `FREECAD_TREE_FIXTURE_OUTPUT` and run a named
test row. For example, from the repository root:

```sh
pixi run --locked env QT_QPA_PLATFORM=offscreen \
  FREECAD_TREE_FIXTURE_OUTPUT=/tmp/new-tree-fixtures \
  build/debug/tests/TreeBehavior_Tests_run idleRestoreCompletes:8-app-open
```

Do not point generation at this data directory. Generated UUIDs, timestamps and
archive metadata can differ. Replacing an input requires reviewing its semantic
structure, updating both test hash constants and this table, and producing fresh
validation evidence. No test saves into the retained input archives.
