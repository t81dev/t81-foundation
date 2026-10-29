# Workflow supply-chain correction

Investigated 2026-10-26 (America/New_York). This is CI portability/security work,
with no production, hypothesis, ISA, API-lock, research-result, or architecture
change. Existing PRs are left unchanged.

## Baseline and reproduction

Fresh remote refs:

| Ref | SHA | State at inspection |
| --- | --- | --- |
| main | `01c85019319fd020922402ba356878e392cb2231` | Unchanged |
| #574 | `10df367102151b087ee64b1fbb737c471f4c87fb` | Open |
| #575 | `b06f8520d02fd68301a5ae06777beb03b73c8eb5` | Open draft, stacked on #574 |
| #576 | `41fa126cc82df6ed54b095d8dd67bfe066252066` | Open draft, stacked on #575 |

The new branch starts at exact #576 head. Protection still requires strict,
up-to-date success from `quality gate / required` and `Analyze (cpp)`;
[protection.json](protection.json) preserves the read-only API result.
There is no merge retry, rule change, existing-PR retarget or rewrite.

Both commands were independently executed from the clean #576 source tree:

```sh
python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0
python3 scripts/ci/audit_workflow_permissions.py --max-missing 0
```

Both exited **1**. Original diagnostics: actions total=108, pinned_sha=101,
tagged=7, unknown=0; tagged references exceed allowed maximum 0. Permissions:
total=20, explicit=19, missing=1, write_scoped=8; qemu-boot-x86.yml is missing
an explicit block. Complete stdout/stderr and every executed argument are in
[before.json](before.json). No threshold changes.

## Seven upstream identities

Line numbers are at exact baseline #576 head. Each version tag or branch was
queried using the authoritative repository's GitHub Git-ref API and separately
`git ls-remote https://github.com/OWNER/REPO.git refs/tags/REF refs/tags/REF^{} refs/heads/REF`.
The selected commit was compared with the exact release's tag, peeling annotated
tag objects if needed. All seven matched. GitHub reports verified signatures
for all seven commits; this is corroboration, not a complete source-code audit.

| Workflow / original line | Repository / mutable ref | Exact release | Verified commit |
| --- | --- | --- | --- |
| docker.yml:33 | docker/setup-buildx-action @ v4 | v4.4.1 | `f87e5991a6d7451dcb8d9637bfbc97413f497069` |
| docker.yml:36 | docker/login-action @ v4 | v4.7.0 | `81c6f93d4bc991e510b342a00b419b3bb4efc5c6` |
| docker.yml:44 | docker/metadata-action @ v6 | v6.2.0 | `dc802804100637a589fabce1cb79ff13a1411302` |
| docker.yml:53 | docker/build-push-action @ v7 | v7.4.0 | `c3c9e263c25d99ce0380d002d59b67737d91b0dc` |
| python-wheels.yml:67 | docker/setup-qemu-action @ v4 | v4.4.0 | `99012661954931238ded8c8b007157a8430204e1` |
| python-wheels.yml:128 | pypa/gh-action-pypi-publish @ release/v1 | v1.14.3 | `7245906fb61fa33a13554f9d5fb856fe61758086` |
| qemu-boot.yml:1175 | actions/upload-artifact @ v7 | v7.0.2 | `cf430e030ddbb5b0abf93d22962f4752f3646cd9` |

`release/v1` is a **branch**, not a tag. The existing auditor calls any
non-immutable @ref "tagged". Both branch and tag substitutions remove mutable
resolution. Chosen pins preserve the refs' actual current upstream resolution;
we do not upgrade to a different major or select an older hash just because it
already appears elsewhere in this repository. Other upload-artifact sites stay
at their existing v7.0.0 pin. Comments retain exact release names, matching the
repository's SHA-plus-version-comment convention; weekly Actions Dependabot
configuration remains unchanged.

[upstream-verification.json](upstream-verification.json) records every upstream
release URL/date, exact tag-object chain, Git transport result, commit URL,
signature verification, action manifest, and published advisory result. Release
sources are the upstream `/releases/tag/VERSION` pages for the seven repositories
listed above. Reproduce a pin using, for example:

```sh
gh api repos/docker/login-action/git/ref/tags/v4
gh api repos/docker/login-action/git/ref/tags/v4.7.0
git ls-remote https://github.com/docker/login-action.git refs/tags/v4 refs/tags/v4.7.0
gh api repos/docker/login-action/commits/81c6f93d4bc991e510b342a00b419b3bb4efc5c6
gh api repos/docker/login-action/security-advisories
```

Mutable refs can move after this audit. Reproduce recorded identities using
the immutable commit and release snapshots rather than assuming a later major
tag lookup must still return the same value.

## Advisory, compatibility and permission review

Public upstream security-advisory API responses were inspected for all seven
repositories. Six had no published advisories in the response; absence is not
proof of freedom from vulnerabilities or a comprehensive transitive audit.
PyPA has two relevant advisories:

- [GHSA-5xjx-3w67-w9x9](https://github.com/pypa/gh-action-pypi-publish/security/advisories/GHSA-5xjx-3w67-w9x9):
  revoked CA-chain trust; affected through 1.14.2, patched from 1.14.3. The
  selected release explicitly contains the repair.
- [GHSA-vxmw-7h4f-hqxh](https://github.com/pypa/gh-action-pypi-publish/security/advisories/GHSA-vxmw-7h4f-hqxh):
  expression expansion injection, affected through 1.12.4. Selected 1.14.3 is
  beyond that range. The existing PyPI trusted-publishing environment/OIDC
  configuration is preserved.

The related Buildx tool advisory
[GHSA-p54p-jq4x-rc28](https://github.com/docker/buildx/security/advisories/GHSA-p54p-jq4x-rc28)
concerns untrusted Bake definitions in Buildx <=0.37.1, patched in 0.37.2;
[the fetched record](buildx-advisory.json) is retained. These workflows use
build-push, not untrusted Bake definitions. Tool/image/dependency downloads
remain a transitive boundary: pinning action commits does not freeze Buildx,
BuildKit, binfmt images, packages, or attest all their code. No compromised
selected release was identified in the checked upstream records. No broader
security certification is claimed.

| Action | Function / runner compatibility | Token requirements in this usage |
| --- | --- | --- |
| setup-buildx | Install/configure Docker builder; Node 24, hosted Ubuntu 24.04 | Public tools; no additional GITHUB_TOKEN scope |
| login | Authenticate GHCR credentials; Node 24, Ubuntu 24.04 | Existing docker job has packages:write for its existing publish operation; unchanged |
| metadata | Generate tags/labels from repository context; Node 24, Ubuntu 24.04 | Existing contents:read sufficient |
| build-push | Build/push image with existing inputs; Node 24, Ubuntu 24.04 | Existing GHCR credentials/packages:write for production push; unchanged |
| setup-qemu | Register aarch64 emulation; Node 24; only Ubuntu aarch64 wheel matrix entry | No new GitHub scope; Docker/privileged binfmt capability already inherent in the action |
| pypi-publish | Linux composite action running Docker, Ubuntu 24.04 publish job | Existing id-token:write for OIDC; no PyPI secret introduced |
| upload-artifact | Store run artifact; Node 24, Ubuntu 24.04 | Actions runtime artifact token, not repository contents write |

Current hosted runners support the Node 24 action runtime already used by the
repository's checkout/setup-python actions; older self-hosted runner support is
not introduced or claimed. PyPA remains in the Linux publishing job, not macOS
or Windows wheel jobs. Conditional aarch64 QEMU behavior remains the same.

## x86 permissions and bounded diff

qemu-boot-x86.yml checks out source, installs host dependencies, configures and
builds a local EFI target, constructs images, boots local QEMU, checks output,
copies a local log, and uploads a run artifact. It contains no git push, GitHub
release/PR mutation, package publication/access, security-events upload, OIDC,
reusable workflow, or job-level permission override. Therefore explicit
`permissions: {contents: read}` is sufficient; all unspecified GITHUB_TOKEN
scopes become none. Artifact upload uses its separate runtime credentials.
See [GitHub permission semantics](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#permissions)
and [least-privilege guidance](https://docs.github.com/en/actions/reference/security/secure-use).

Production workflow changes are seven verified SHA substitutions and the single
read-only permission block. Supporting validation adds six regression methods,
reviewed pin/permission fixtures, PyYAML 6.0.3 to existing CI Python tools, and
a manual-only smoke job inside existing ci.yml. Existing production job
triggers, inputs, outputs, dependencies, write grants and publish conditions
remain unchanged. No new workflow file or required-check dependency is added.

The smoke job has contents:read only, no secrets, no OIDC and no upstream gate
dependency. It runs all seven exact pins against dummy credentials, a scratch
image, a disposable wheel and runner-local HTTP endpoints. Build push is false;
PyPA upload is directed to the local Docker bridge, with attestations disabled
for dummy-password authentication. Its artifact is uploaded to this workflow
run. This proves action execution for bounded test inputs; it does not prove
GHCR push, real PyPI OIDC publication, or a full QEMU OS boot.

## Regression and independent inventory results

```sh
python3 tests/ci/workflow_supply_chain_test.py
python3 scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0
python3 scripts/ci/audit_workflow_permissions.py --max-missing 0
python3 scripts/ci/test_failure_propagation.py
python3 tests/ci/check_architecture_targets_test.py
python3 tests/ci/governance_deadlock_test.py
python3 scripts/ci/check_tisc_freeze_integrity.py
```

New tests require PyYAML 6.0.3 (local installed version, also pinned in CI).
Six methods pass. Seven negative @ref cases reject major tags, release branches,
39/41-character hashes, nonhex hashes, missing @ref and unknown refs. Removing
explicit permissions fails the original audit. A separate YAML-parsed budget
rejects four token-expansion cases including a job-level write override and
write-all. The original permissions audit measures explicitness, not least
privilege; its zero result alone is not the permission-budget proof. The budget
snapshot preserves all previous workflow/job grants and allows only x86
contents:read and the new smoke job's contents:read.

Actions after: **116 references, all 116 pinned, 0 tagged, 0 unknown**. The
extra eight references are seven bounded smoke invocations plus its checkout;
the original 108 references are all pinned. Workflows remain **20**, now all
20 explicit, missing=0; write-scoped workflows remain **8**.

Existing suites pass: 18 architecture-target tests, five governance methods
(including their 20 invariant mutations), 47 failure-propagation controls,
and 209-opcode freeze integrity. Full local outcomes are preserved in
[after-and-tests.json](after-and-tests.json). Local environment: macOS ARM64,
Python 3.14.3. These static changes do not modify production runtime behavior;
no local production build is claimed.

All 27 previous governance commands were independently rerun, with captured
stdout/stderr and timeout=180. The predicted improvement was observed:
**16 passing / 11 failing -> 18 passing / 9 failing**. The complete
[before/after table](inventory.md) and raw JSON preserve every command. No skip
is counted as a local pass; the separately running metrics generator also ran.
Translation staleness depends on full Git history; hosted shallow results can
differ. This inventory precedes the new audit commit and is labeled accordingly.

## Hosted evidence and integration limits

The existing ci.yml is manually dispatched on the pushed exact corrective head.
The draft PR description records its assigned number, exact head, workflow run
and job links, completed results, skips, untriggered checks and diagnostic
artifacts. The read-only x86 workflow can also run independently; any existing
build/boot failures are recorded rather than repaired here. Publishing workflows
are not dispatched against real package registries, and AArch64's automatic
boot-log commit job is not used to mutate this branch.

Next spec/docs blocker is `scripts/governance/check_root_structure.py`. Nine
remaining failing commands cover root structure; translation metadata/staleness;
docs structure; API version/digest lock; spec-code mappings; status labels;
RFC metadata; and aggregate governance hygiene. They remain untouched. Downstream
required jobs remain unproven when skipped. CodeQL must be read independently;
its absence from a stacked PR's check list is not success.

Recommended sequence: independently review this exact draft; investigate the
remaining nine failures under their own contracts, beginning with root-layout
authority; then execute all thirteen quality-gate dependency jobs and CodeQL
on a cumulative integration candidate. Descendant fixes cannot repair #574's
unchanged own-head checks. A later maintainer-authorized cumulative main-targeted
PR, or authorized prerequisite incorporation and renewed review of #574, is
needed to resolve that graph constraint. Neither is performed here. Preserve
normal protections and all negative scientific findings.
