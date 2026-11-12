---
title: Assembly motion suspension delivery follow-ups
doc_type: reference
artifact_type: reference
status: active
owner: local-developer
last_reviewed: 2026-11-12
---

# Assembly Delivery Follow-ups

## B015 View test fixtures

Status: proposed. Owner: local-developer. Source: Spec006 verification E001.
Repair unchanged TestCommandCreateView _Move mocks so the distance/angle row-edit tests provide
Label expected by existing CommandCreateView.rebuildStepList. Both failures reproduce in isolation;
all three involved files match source base3a53ef2ce7. Exit: both assertions and the full Assembly
suite pass without weakening production view behavior. This is separate scope, not feature work.

## B016 Upstream and bridge qualification

Status: proposed. Owner: local-developer (FreeCAD delivery); MCP owner (integration qualification).
Use the documented API and exact build identity to adapt/qualify the bridge, including its
independent receipt/recovery obligations. Before public PR prep obtain human code understanding,
review/testing, issue/API agreement, Windows/macOS CI and manual drag checks. Select a focused
upstream source base rather than submitting the integration ancestry. Exit: owned qualification
records and personally authored PR preparation; no automatic merge/push is authorized here.
