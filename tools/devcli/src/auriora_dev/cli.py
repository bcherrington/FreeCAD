from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path

import typer


app = typer.Typer(
    no_args_is_help=True,
    help="Stable developer CLI for the lite project template.",
)
spec_app = typer.Typer(help="Work with visible repo spec files.")
integration_app = typer.Typer(help="Manage local integration-test branch merges.")
app.add_typer(spec_app, name="spec")
app.add_typer(integration_app, name="integration")


def _repo_root() -> Path:
    return Path(__file__).resolve().parents[4]


def _integration_state_path() -> Path:
    return _repo_root() / ".git" / "personal-integration-branches.json"


def _run_git(*args: str, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        ["git", *args],
        cwd=_repo_root(),
        check=check,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )


def _git_output(*args: str) -> str:
    return _run_git(*args).stdout.strip()


def _git_passthrough(*args: str) -> None:
    try:
        subprocess.run(["git", *args], cwd=_repo_root(), check=True)
    except subprocess.CalledProcessError as exc:
        raise typer.Exit(code=exc.returncode) from exc


def _require_clean_worktree() -> None:
    status = _git_output("status", "--porcelain")
    if status:
        typer.secho(
            "Working tree is not clean. Commit, stash, or discard changes first.",
            fg=typer.colors.RED,
        )
        raise typer.Exit(code=1)


def _current_branch() -> str:
    return _git_output("branch", "--show-current")


def _branch_exists(ref: str) -> bool:
    return _run_git("rev-parse", "--verify", "--quiet", ref, check=False).returncode == 0


def _default_integration_state() -> dict[str, object]:
    return {
        "base": "upstream/main",
        "branches": [],
    }


def _load_integration_state() -> dict[str, object]:
    path = _integration_state_path()
    if not path.exists():
        return _default_integration_state()

    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict):
        raise typer.BadParameter(f"{path} must contain a JSON object.")

    base = data.get("base", "upstream/main")
    branches = data.get("branches", [])
    if not isinstance(base, str) or not isinstance(branches, list):
        raise typer.BadParameter(f"{path} has an invalid integration branch format.")

    return {
        "base": base,
        "branches": [branch for branch in branches if isinstance(branch, str)],
    }


def _save_integration_state(state: dict[str, object]) -> None:
    path = _integration_state_path()
    path.write_text(json.dumps(state, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _tracked_integration_branches(state: dict[str, object]) -> list[str]:
    branches = state.get("branches", [])
    if not isinstance(branches, list):
        return []
    return [branch for branch in branches if isinstance(branch, str)]


def _resolve_branch_selections(selections: list[str], branches: list[str]) -> list[str]:
    if not selections:
        return branches

    resolved: list[str] = []
    for selection in selections:
        branch = ""
        if selection.isdigit():
            index = int(selection)
            if index < 1 or index > len(branches):
                raise typer.BadParameter(f"Branch selection {selection} is out of range.")
            branch = branches[index - 1]
        else:
            branch = selection
            if branch not in branches:
                raise typer.BadParameter(
                    f"{branch} is not tracked. Use 'proj integration add {branch}'."
                )

        if branch not in resolved:
            resolved.append(branch)

    return resolved


def _merge_ref(ref: str) -> None:
    typer.secho(f"Merging {ref}", fg=typer.colors.CYAN)
    _git_passthrough("merge", "--no-edit", ref)


def _local_branches() -> list[str]:
    output = _git_output("for-each-ref", "--format=%(refname:short)", "refs/heads")
    if not output:
        return []
    return [line.strip() for line in output.splitlines() if line.strip()]


def _spec_paths() -> dict[str, Path]:
    root = _repo_root()
    return {
        "requirements": root / "docs" / "spec" / "requirements.md",
        "design": root / "docs" / "spec" / "design.md",
        "tasks": root / "docs" / "spec" / "tasks.md",
    }


def _spec_dir_paths() -> dict[str, Path]:
    root = _repo_root()
    return {
        "requirements_dir": root / "docs" / "spec" / "requirements",
        "design_dir": root / "docs" / "spec" / "design",
        "tasks_dir": root / "docs" / "spec" / "tasks",
    }


def _placeholder(name: str, detail: str) -> None:
    typer.secho("Template Placeholder", fg=typer.colors.CYAN, bold=True)
    typer.echo(f"{name}: {detail}")


def _slugify(value: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "-", value.strip().lower())
    slug = slug.strip("-")
    if not slug:
        raise typer.BadParameter("Could not derive a usable slug from the input.")
    return slug


def _write_if_missing(path: Path, content: str) -> None:
    if path.exists():
        raise typer.BadParameter(f"{path.relative_to(_repo_root())} already exists.")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


@app.command()
def setup() -> None:
    """Install or verify local development prerequisites."""
    _placeholder(
        "setup",
        "Replace this command with project-specific environment setup steps.",
    )


@app.command()
def dev() -> None:
    """Run the project locally."""
    _placeholder(
        "dev",
        "Replace this command with the local run workflow for the project.",
    )


@app.command()
def lint() -> None:
    """Run formatting and lint checks."""
    _placeholder(
        "lint",
        "Replace this command with project-specific lint and formatting checks.",
    )


@app.command()
def test() -> None:
    """Run automated tests."""
    _placeholder(
        "test",
        "Replace this command with project-specific automated tests.",
    )


@spec_app.command("show")
def spec_show() -> None:
    """Show the visible spec files used by the template."""
    paths = _spec_paths()
    typer.secho("Visible spec files", bold=True)
    for name, path in paths.items():
        exists = "present" if path.exists() else "missing"
        typer.echo(f"- {name}: {path.relative_to(_repo_root())} [{exists}]")

    dir_paths = _spec_dir_paths()
    typer.echo("")
    typer.secho("Optional split spec directories", bold=True)
    for name, path in dir_paths.items():
        exists = "present" if path.exists() else "not in use"
        typer.echo(f"- {name}: {path.relative_to(_repo_root())} [{exists}]")


@spec_app.command("check")
def spec_check() -> None:
    """Fail if any of the default spec files are missing."""
    missing = [path for path in _spec_paths().values() if not path.exists()]
    if missing:
        for path in missing:
            typer.secho(
                f"Missing {path.relative_to(_repo_root())}",
                fg=typer.colors.RED,
            )
        raise typer.Exit(code=1)

    typer.secho("All default spec files are present.", fg=typer.colors.GREEN)
    typer.echo(
        "Keep requirements, design, tasks, and code consistent as work evolves."
    )


@spec_app.command("scaffold-split")
def spec_scaffold_split(
    kind: str = typer.Argument(..., help="One of: requirements, design"),
) -> None:
    """Create a split requirements or design directory with an index template."""
    root = _repo_root()
    normalized = kind.strip().lower()
    if normalized not in {"requirements", "design"}:
        raise typer.BadParameter("kind must be 'requirements' or 'design'.")

    split_dir = root / "docs" / "spec" / normalized
    index_path = split_dir / "README.md"

    title = "Requirements" if normalized == "requirements" else "Design"
    content = f"""# {title}

Use this directory when one `{normalized}.md` file is no longer enough.

## Suggested organization

- one file per feature, domain, or subsystem
- one overview or index entry in this file
- keep names short and obvious

## Files

- [{title} overview](../{normalized}.md)

Add focused files here as the project grows.
"""

    if index_path.exists():
        typer.secho(
            f"{index_path.relative_to(root)} already exists.",
            fg=typer.colors.YELLOW,
        )
        raise typer.Exit(code=0)

    _write_if_missing(index_path, content)
    typer.secho(
        f"Created {index_path.relative_to(root)}",
        fg=typer.colors.GREEN,
    )


@spec_app.command("new-task")
def spec_new_task(
    title: str = typer.Argument(..., help="Short title for the task file"),
    slug: str | None = typer.Option(
        None,
        "--slug",
        help="Optional slug override for the filename.",
    ),
) -> None:
    """Create a new short-lived checklist task file under docs/spec/tasks/."""
    root = _repo_root()
    task_slug = slug or _slugify(title)
    task_path = root / "docs" / "spec" / "tasks" / f"{task_slug}.md"

    content = f"""# {title}

Status: Active

Use this file for a short-lived implementation effort.
Task markers:

- [ ] not started
- [-] in progress
- [x] done

## Scope

- describe the feature, refactor, bug fix, or migration

## Phase 1: Scope and alignment

- [x] 1. Confirm the problem and scope
- [-] 1.1 Align requirements and design
  - Document the required behavior
  - Confirm the design still matches the code
  - _Requirements: 1.1, 1.2_
- [ ] 1.2 Identify verification needs

## Phase 2: Implementation

- [ ] 2. Update the code
- [ ] 2.1 Add or update tests
- [ ] 2.2 Verify the behavior end to end

## Phase 3: Close out

- [ ] 3. Update requirements if needed
- [ ] 3.1 Update design if needed
- [ ] 3.2 Mark all completed items accurately

## Notes

- update requirements and design if this task changes them
- keep code, requirements, and design consistent before closing this file
"""

    _write_if_missing(task_path, content)
    typer.secho(
        f"Created {task_path.relative_to(root)}",
        fg=typer.colors.GREEN,
    )
    typer.echo("Use docs/spec/tasks.md for the current default workflow, or use")
    typer.echo("docs/spec/tasks/ when you want short-lived task history.")


@integration_app.command("list")
def integration_list() -> None:
    """Show the locally tracked integration merge branches."""
    state = _load_integration_state()
    base = str(state.get("base", "upstream/main"))
    branches = _tracked_integration_branches(state)

    typer.secho("Integration branch state", bold=True)
    typer.echo(f"current branch: {_current_branch()}")
    typer.echo(f"base: {base}")
    typer.echo(f"state file: {_integration_state_path().relative_to(_repo_root())}")
    typer.echo("")

    if not branches:
        typer.echo("No tracked merge branches yet.")
        return

    for index, branch in enumerate(branches, start=1):
        commit = (
            _git_output("rev-parse", "--short", branch)
            if _branch_exists(branch)
            else "missing"
        )
        typer.echo(f"{index}. {branch} [{commit}]")


@integration_app.command("available")
def integration_available(
    all_branches: bool = typer.Option(
        False,
        "--all",
        help="Include the current branch and branches already tracked for integration.",
    ),
) -> None:
    """List local branches that can be added to integration tracking."""
    state = _load_integration_state()
    tracked = set(_tracked_integration_branches(state))
    current = _current_branch()
    branches = _local_branches()

    available = branches if all_branches else [
        branch for branch in branches if branch != current and branch not in tracked
    ]

    if not available:
        typer.echo("No available local branches.")
        return

    for index, branch in enumerate(available, start=1):
        commit = _git_output("rev-parse", "--short", branch)
        marker = ""
        if branch == current:
            marker = " [current]"
        elif branch in tracked:
            marker = " [tracked]"
        typer.echo(f"{index}. {branch} [{commit}]{marker}")


@integration_app.command("base")
def integration_base(
    ref: str = typer.Argument(
        ...,
        help="Base ref to merge before tracked branches, usually upstream/main.",
    ),
) -> None:
    """Set the base ref merged before integration branches."""
    if not _branch_exists(ref):
        raise typer.BadParameter(f"{ref} does not exist locally.")

    state = _load_integration_state()
    state["base"] = ref
    _save_integration_state(state)
    typer.secho(f"Integration base set to {ref}.", fg=typer.colors.GREEN)


@integration_app.command("add")
def integration_add(
    branch: str = typer.Argument(..., help="Branch or ref to merge and track."),
    merge: bool = typer.Option(
        True,
        "--merge/--no-merge",
        help="Merge the branch immediately.",
    ),
) -> None:
    """Merge a branch into the current integration branch and track it for later updates."""
    if not _branch_exists(branch):
        raise typer.BadParameter(f"{branch} does not exist locally.")

    state = _load_integration_state()
    branches = _tracked_integration_branches(state)
    if branch in branches:
        typer.secho(f"{branch} is already tracked.", fg=typer.colors.YELLOW)
    else:
        branches.append(branch)
        state["branches"] = branches

    if merge:
        _require_clean_worktree()
        _merge_ref(branch)

    _save_integration_state(state)
    typer.secho(f"Tracking {branch}.", fg=typer.colors.GREEN)


@integration_app.command("update")
def integration_update(
    selections: list[str] | None = typer.Argument(
        None,
        help="Optional branch names or numbers from 'proj integration list'. Omit to update all.",
    ),
    include_base: bool = typer.Option(
        True,
        "--base/--no-base",
        help="Merge the configured base before selected branches.",
    ),
    fetch: bool = typer.Option(
        False,
        "--fetch",
        help="Run 'git fetch --all --prune' first.",
    ),
) -> None:
    """Merge the base and all or selected tracked branches into the current branch."""
    _require_clean_worktree()

    state = _load_integration_state()
    base = str(state.get("base", "upstream/main"))
    branches = _tracked_integration_branches(state)
    selected = _resolve_branch_selections(selections or [], branches)

    if fetch:
        typer.secho("Fetching remotes", fg=typer.colors.CYAN)
        _git_passthrough("fetch", "--all", "--prune")

    if include_base:
        if not _branch_exists(base):
            raise typer.BadParameter(f"Configured base {base} does not exist locally.")
        _merge_ref(base)

    if not selected:
        typer.echo("No tracked branches selected.")
        return

    for branch in selected:
        if not _branch_exists(branch):
            raise typer.BadParameter(f"Tracked branch {branch} does not exist locally.")
        _merge_ref(branch)

    typer.secho("Integration branch update complete.", fg=typer.colors.GREEN)


@integration_app.command("remove")
def integration_remove(
    selections: list[str] = typer.Argument(
        ...,
        help="Branch names or numbers from 'proj integration list' to stop tracking.",
    ),
) -> None:
    """Stop tracking branches for future integration updates."""
    state = _load_integration_state()
    branches = _tracked_integration_branches(state)
    remove = set(_resolve_branch_selections(selections, branches))
    state["branches"] = [branch for branch in branches if branch not in remove]
    _save_integration_state(state)
    typer.secho(f"Stopped tracking {len(remove)} branch(es).", fg=typer.colors.GREEN)


def main() -> None:
    app()


if __name__ == "__main__":
    main()
