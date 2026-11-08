# T004.2 bounded runtime-receipt forensics

Date: 2026-11-08. Owner: local-developer. Status: planned technical correction;
not a performance campaign or acceptance waiver.

The fixed P07 diagnostic under freeze
`800b7b11d9dbd8ba2e5fec422400238e19c9e6f98c34572718ccdebf0eb13f73`
stopped at worker 0 with `Loaded arm libraries differ from qualification`.
One eager worker was attempted, zero qualified observations were published and
the other 15 workers remain unexecuted. Do not resume or replace that campaign.
Its post-abort audit passed all 39,964 bindings; its host trace and failure output
are retained. The error result omitted the observed module difference, so its
exact cause cannot be recovered from that worker.

## Concrete correction

A source-only probe correction retains build-relative observed/expected module
hashes and missing/additional/changed names on a runtime-receipt exception.
It preserves the existing comparison and failure; it does not permit additional
modules or weaken a fidelity/provenance check. Two focused unit checks verify
successful identity and failing difference preservation.

## Separate bounded qualification

Run at most **two** new fresh GUI processes: one eager, then one retained-lazy,
using the hidden primary fixture. These are forensic qualification, not replacement
performance observations. Do not assess, pool or publish their timing values.
Stop after these two, or immediately on an unexpected failure.

- Keep the same native binaries, source/dependency composition, native X11 profile,
  viewport, camera, addon, private fixture and old full model/pixel/count witnesses.
- Bind new Python-only source commits; prove the native `src` trees and native
  trace/test C++ sources match the final 408-case qualification. No C++ rebuild
  or new native performance change is selected.
- Use independent small Mesa/XDG copies of the unchanged lazy-arm initial
  eight-file cache seed. Verify it against the stopped campaign's initial hashes.
  Keep every stopped campaign artifact and source ref. Record the worktree source
  advancement after its successful post-abort audit.
- Recompute namespace/context identities for these exact new commits/cache roots.
  Retain the old module expectations deliberately: the new exception receipt
  should reveal the difference while preserving the original failing check.
- Enforce 60 seconds and 256 KiB streams per process, four-minute total ceiling,
  private output and pidfd-owned cleanup. No extra/replacement process or retry.
- A runtime-receipt mismatch with the new structured receipt is an expected
  forensic outcome. Every observed mapped build file must match the full static
  qualified output manifest. Any other error or unbound/hash-mismatched file
  stops the qualification. Preserve both successful and failing results.

Compare the actual missing/additional/changed mappings across arms. Classify any
new module/resource before choosing a correction. Do not broaden an expected
module set merely to obtain a passing measurement. Another timing campaign needs
a separately recorded reasoned plan, new freeze and runtime qualification; this
plan authorizes only the two bounded forensic processes.
