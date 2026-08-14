---
title: Integration guide title
doc_type: integration
status: draft
owner: team-or-person
last_reviewed: YYYY-MM-DD
---

# Title

## Purpose

Describe the runtime integration and why it exists.

## Integration Surface

Summarize the local or external surface being integrated, such as an MCP tool
or resource, language adapter, parser, LSP, validation command, plugin, editor
client, or storage component.

## Contract Summary

Describe the tool/resource schemas, request and response shapes, config files,
workspace files, commands, or storage records involved.

## Contract Status

| Artifact | Status | Notes |
| --- | --- | --- |
| MCP schema, JSON schema, CLI contract, parser output, LSP capability, or storage schema | available, repo-owned, captured in tests, pending design, or not applicable | State where the authoritative contract lives. |

## Runtime Identity

| Identity | Value | Notes |
| --- | --- | --- |
| Integration name | integration-name | Stable name used in docs, config, logs, and diagnostics. |
| Runtime area | mcp, language-adapter, parser, lsp, validation, plugin, storage, or client | Primary subsystem that owns the integration. |
| Entry point | tool, resource, command, watcher event, adapter method, schema, or table | How the runtime invokes or observes this integration. |
| Capability level | semantic, partial_semantic, resource_backed, unsupported, or custom | Capability reported to agents when applicable. |

## Trust And Access

Describe local trust boundaries, workspace permissions, process execution,
network use, credential handling, or sandbox assumptions.

## Configuration

List repo config files, runtime settings, environment variables, command-line
flags, plugin metadata, or adapter registration settings that control the
integration.

## Linked Docs

| Document | Why It Matters |
| --- | --- |
| Related architecture or design doc | Explains the subsystem boundary and runtime behavior. |
| Related runbook | Describes validation, refresh, recovery, or troubleshooting procedure. |
| Related contract, schema, or reference doc | Explains request, response, storage, config, or capability constraints. |

## Runtime Flow

Describe how requests, workspace events, indexed facts, diagnostics, or command
results move through the integration.

## Surface Mapping

| Input Or Trigger | Runtime Entry Point | State Read | Output Or Side Effect | Validation |
| --- | --- | --- | --- | --- |
| MCP request, file change, command invocation, or adapter call | Tool, resource, watcher, parser, LSP, or validator | Files, graph store, cache, config, or process output | Tool response, graph update, diagnostics, edit preview, or validation result | Test, fixture, command, or manual check |

## Failure Modes

- Failure:
  Cause, detection, and likely impact.

## Operations

Summarize normal operator or support tasks for this integration.

## Validation Evidence

Describe the tests, commands, fixtures, schema checks, or runtime smoke checks
that prove the integration still works.

## Change Process

Describe which runbook or checklist to use when changing schemas, adapter
capabilities, parser behavior, commands, config keys, storage shape, or agent
visible responses.

## References

- External or upstream specs:
- Internal runbooks:
- Related architecture docs:
