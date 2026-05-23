# Dev CLI

This package provides the default developer interface for the template.

## Install

```bash
pip install --no-build-isolation -e tools/devcli
```

## Commands

```bash
proj setup
proj dev
proj lint
proj test
proj spec show
proj spec scaffold-split requirements
proj spec new-task "fix login timeout"
proj integration list
proj integration available
proj integration add compact-ui-rails-for-panels
proj integration update
proj integration update 1 3
proj integration update compact-ui-title-bar --no-base
```

Replace the placeholder command implementations in `src/auriora_dev/cli.py` with project-specific behavior.
Task files use grouped Kiro-style checklists with `[ ]`, `[-]`, and `[x]`, plus numbered tasks and sub-tasks.

## Integration branch helper

Use `proj integration` from a local integration branch when you want one branch
that merges several personal or external PR branches for combined builds and
manual testing. The helper stores its local tracking state in
`.git/personal-integration-branches.json`, so it is specific to this checkout
and is not committed.

- `proj integration add <branch>` merges a new branch and tracks it for future
  updates.
- `proj integration available` lists local branches that are not already
  tracked and are not the current integration branch.
- `proj integration update` merges the configured base, then every tracked
  branch.
- `proj integration update 1 3` merges only selected branches by their numbered
  entries from `proj integration list`.
- `proj integration update <branch>` merges one tracked branch by name.
- `proj integration base <ref>` changes the base ref merged before tracked
  branches. The default is `upstream/main`.
- `proj integration remove <selection>` stops tracking a branch.

The helper refuses to start a merge while the working tree is dirty. If a merge
conflicts, resolve it with normal Git commands, commit the integration result,
then re-run the helper as needed.
