# AXION-AI-001 ADR-001 — Candidate Preparation Validation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [AXION-AI-001 ADR-001 — Candidate Preparation Validation](#axion-ai-001-adr-001-—-candidate-preparation-validation)
  - [Scope and workflow](#scope-and-workflow)
  - [Corrections and manual consistency inspection](#corrections-and-manual-consistency-inspection)
  - [Results and limits](#results-and-limits)
  - [Commands, outputs and source identities](#commands-outputs-and-source-identities)
  - [Reproduction of the preparation-only check](#reproduction-of-the-preparation-only-check)

<!-- T81-TOC:END -->


Status: Non-normative documentation evidence only
Date: 2026-11-17
Base: `56db3a18909a029042f7ceded2e19d96aec57cdc`

Collector: Codex, candidate author with prior proposal-authoring context. This is
not independent scientific validation or a Gemini review. The user's supplied
adversarial review/authorization are attributed in the ADR. No implementation,
host work, benchmark, model inference, proof checker or acceptance scenario ran.
No packages were installed. All behavioral criteria remain PROPOSED / unexecuted.

## Scope and workflow

Exactly two new documentation files, listed below; no existing tracked bytes,
active RFC/protocol/schema, implementation, test, lock, gate or qualification
artifact changed. The new records file is an audit of preparation, not an active
governance artifact. The author used an isolated branch/worktree to preserve
unrelated untracked artifacts in the user's original checkout.

The ADR follows ADR_TEMPLATE and ARCHITECTURE_DOC_STANDARD: Proposed status,
Experimental maturity, purpose/scope, implementation boundary, decision,
alternatives, invariants, failure modes, Indeterminate, evidence, acceptance and
versioning. Repository ADR-0007 retains research-local AXION-AI-001 ADR-001.
CONTRIBUTING.md section 2 requires PR-based contribution review; section 3 allows
relevant documentation checks for documentation-only changes. A separate draft
PR is the review vehicle. No separate issue ID was supplied with the human task;
the PR records that provenance rather than inventing an issue or reviewer result.

## Corrections and manual consistency inspection

- Correction 1: section 6 separates dispatch progress from effect outcome.
  INTENT_DURABLE is distinct from HANDOFF_ARMED / HANDOFF_CONFIRMED and
  NO_HANDOFF_PROVEN. D(e) is explicitly the committed entry into possible
  handoff, not proof of physical send. Missing/torn/partial/ambiguous markers
  cannot imply absence. Unresolved possible dispatch recovers UNCERTAIN;
  unidentified exposure retains session INCOMPLETE and dispatch STOP. I13
  covers each persistence/handoff crash boundary and damaged journal tails.
  Cancellation, request acceptance and local termination are not absence proof;
  no exactly-once remote execution is asserted.
- Correction 2: section 7 binds replacement admission to exact destination
  rendering/tokenization and all template, tools, attachments, special/protocol
  tokens and reserved-output overhead. Over-budget/unprovable fit gives
  ContextBudgetExceeded unless separately authorized, provenance-preserving
  compaction creates a retained child state passing fresh checks. New proposed
  StateTransformationRef / ContextAdmissionRef lists retain evidence, not tokens
  or authority. I09 covers exact-fit/one-over components, altered identities,
  silent truncation and incompatible caches. All compaction work is charged.
- I01-I13 each retain an adversarial counterexample and observable rejection,
  history/identity/accounting mismatch or forbidden effect as a falsifier. The
  state-machine clauses, I08/I13, I09/schema outline and aggregate accounting
  were read together by the author. This is source-level consistency inspection;
  it does not prove state-machine correctness or execute an independent oracle.
- The remote gateway is explicitly a separate trusted constrained executor.
  Untrusted workers receive no ambient egress or privileged effect credentials.
- HD01-HD06 remain unresolved human gates; preferred research architecture and
  drafting approval do not resolve them. BOLQ NOT ADOPTED; residual risks NOT
  ACCEPTED; M1–M6 STOP; linux_relock_required=true. Existing F01 protocol hash
  and qualification semantics remain unchanged.

## Results and limits

54 static checks passed: architecture structure/classification/boundary markers,
I01-I13 and six HD gates, two state axes, context clauses, six immutable Git-blob
source identities/line ranges, 12 local links, Markdown fences and exact two-file
scope. The closed RFC-00D3 schema is read from its pinned Git object; it is not
modified. Active-doc legacy-link hygiene and whitespace checks passed.

The static check initially had an incorrect matrix-cell delimiter assertion;
that preparation-helper assertion was corrected and the complete check rerun.
This was not a changed ADR contract or an executed behavioral counterexample.
The initial staged whitespace check identified Markdown hard-break trailing
spaces; those were removed and the staged check rerun successfully.

Omitted: C++ configure/build/tests (documentation-only scope), runtime probes,
model/worker/host execution, fault injection, schema-instance conformance,
scientific/benchmark experiments and external-service calls. The ABI and
PortableTaskState are typed outlines, not runnable schemas. Mermaid source was
inspected; no renderer was installed or executed. Text checks cannot establish
semantic correctness, independent scientific review or host guarantees.

## Commands, outputs and source identities

These identify the pre-commit working tree by ADR hash and exact baseline.
Resolve the publication commit through this file's Git history; no hypothetical
future or self-referential commit identity is used. This record's own identity
is supplied by the resulting commit.

```json
{
  "commands": [
    {
      "command": [
        "python3",
        "-B",
        "/private/tmp/t81-axion-adr-static-review.py"
      ],
      "working_directory": "/private/tmp/t81-axion-ai-001-adr",
      "exit": 0,
      "stdout": "PASS: 54 static documentation checks; 6 immutable source identities; 12 local links; I01-I13 and HD01-HD06 present; exact two-file scope\n",
      "stderr": ""
    },
    {
      "command": [
        "python3",
        "-B",
        "scripts/governance/check_active_docs_link_hygiene.py"
      ],
      "working_directory": "/private/tmp/t81-axion-ai-001-adr",
      "exit": 0,
      "stdout": "active docs link hygiene check PASSED\n- no legacy docs/guides path references found on the active docs surface\n",
      "stderr": ""
    },
    {
      "command": [
        "git",
        "diff",
        "--cached",
        "--check"
      ],
      "working_directory": "/private/tmp/t81-axion-ai-001-adr",
      "exit": 0,
      "stdout": "",
      "stderr": ""
    }
  ],
  "static_report": {
    "base": "56db3a18909a029042f7ceded2e19d96aec57cdc",
    "python": "3.14.3 (main, Feb  3 2026, 15:32:20) [Clang 17.0.0 (clang-1700.6.3.2)]",
    "working_directory": "/private/tmp/t81-axion-ai-001-adr",
    "checks": {
      "baseline commit": "PASS",
      "architecture header and maturity": "PASS",
      "ADR and architecture-standard sections": "PASS",
      "all I01-I13 rows exactly once": "PASS",
      "each invariant has counterexample and acceptance/falsifier cells": "PASS",
      "six human gates exactly once": "PASS",
      "classification vocabulary": "PASS",
      "proposal and no-execution boundary": "PASS",
      "dispatch and recovery terminology": "PASS",
      "all original effect outcome states": "PASS",
      "context-shrinkage and provenance clauses": "PASS",
      "F01 holds and protocol digest retained": "PASS",
      "reference definition E01": "PASS",
      "reference definition E02": "PASS",
      "reference definition E03": "PASS",
      "reference definition E04": "PASS",
      "reference definition E05": "PASS",
      "reference definition E06": "PASS",
      "reference definition validation": "PASS",
      "reference-style labels resolve": "PASS",
      "local link ../../research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md": "PASS",
      "local link ../../research/RESEARCH_CHARTER.md": "PASS",
      "local link ../../research/continuity/HYPOTHESIS_REGISTER_v1.json": "PASS",
      "local link ../../governance/SPEC_AUTHORITY_MODEL.md": "PASS",
      "local link ../../governance/FREEZE_ENFORCEMENT.md": "PASS",
      "local link ../../governance/DETERMINISM_SURFACE_REGISTRY.md": "PASS",
      "local link ../../governance/DETERMINISM_THREAT_MODEL.md": "PASS",
      "local link ../../product/DETERMINISTIC_CORE_PROFILE.md": "PASS",
      "local link ADR_TEMPLATE.md": "PASS",
      "local link ../ARCHITECTURE_DOC_STANDARD.md": "PASS",
      "local link ../../research/proposals/RFC-00D7-F01-BOLQ-MIGRATION/REVIEW_PLAN.md": "PASS",
      "immutable source URL https://github.com/t81dev/t81-foundation/blob/56db3a18909a029042f7ceded2e19d96aec57cdc/kernel/axion/axion_api.cpp#L8": "PASS",
      "source pin kernel/axion/axion_api.cpp": "PASS",
      "source line anchor kernel/axion/axion_api.cpp": "PASS",
      "immutable source URL https://github.com/t81dev/t81-foundation/blob/56db3a18909a029042f7ceded2e19d96aec57cdc/tools/model/llama_cpp_adapter.cpp#L214": "PASS",
      "source pin tools/model/llama_cpp_adapter.cpp": "PASS",
      "source line anchor tools/model/llama_cpp_adapter.cpp": "PASS",
      "immutable source URL https://github.com/t81dev/t81-foundation/blob/56db3a18909a029042f7ceded2e19d96aec57cdc/fs/block_backed_driver.cpp#L142": "PASS",
      "source pin fs/block_backed_driver.cpp": "PASS",
      "source line anchor fs/block_backed_driver.cpp": "PASS",
      "immutable source URL https://github.com/t81dev/t81-foundation/blob/56db3a18909a029042f7ceded2e19d96aec57cdc/spec/rfcs/RFC-00D3-model-manifest-schema.json#L6": "PASS",
      "source pin spec/rfcs/RFC-00D3-model-manifest-schema.json": "PASS",
      "source line anchor spec/rfcs/RFC-00D3-model-manifest-schema.json": "PASS",
      "immutable source URL https://github.com/t81dev/t81-foundation/blob/56db3a18909a029042f7ceded2e19d96aec57cdc/vm/vm.cpp#L4010": "PASS",
      "source pin vm/vm.cpp": "PASS",
      "source line anchor vm/vm.cpp": "PASS",
      "immutable source URL https://github.com/t81dev/t81-foundation/blob/56db3a18909a029042f7ceded2e19d96aec57cdc/ternaryos/kernel/kernel_abi.hpp#L23": "PASS",
      "source pin ternaryos/kernel/kernel_abi.hpp": "PASS",
      "source line anchor ternaryos/kernel/kernel_abi.hpp": "PASS",
      "local link ../../records/audits/2026-11-17-axion-ai-001-adr-001/VALIDATION.md": "PASS",
      "balanced Markdown fences": "PASS",
      "closed D3 schema unchanged": "PASS",
      "exact two-file documentation-only scope": "PASS",
      "no baseline tracked files modified": "PASS"
    },
    "pinned_sources": [
      {
        "commit": "56db3a18909a029042f7ceded2e19d96aec57cdc",
        "path": "kernel/axion/axion_api.cpp",
        "sha256": "a9735db4ef38af8811bbf91389f07eda887b1ef6ca1378675953abb7c87aefca"
      },
      {
        "commit": "56db3a18909a029042f7ceded2e19d96aec57cdc",
        "path": "tools/model/llama_cpp_adapter.cpp",
        "sha256": "e4b97ae1de4cea24338f0f9d23e218225a9e3cbb8493f6d237a319a1b5491043"
      },
      {
        "commit": "56db3a18909a029042f7ceded2e19d96aec57cdc",
        "path": "fs/block_backed_driver.cpp",
        "sha256": "b5bceb6eac7cdf2c0cf6abd34b23485794da9da9698c5d006c2f853fe14751b4"
      },
      {
        "commit": "56db3a18909a029042f7ceded2e19d96aec57cdc",
        "path": "spec/rfcs/RFC-00D3-model-manifest-schema.json",
        "sha256": "a7c2cf69c9fc3b2030075cca2ea902fc5c28c53ebd28dc80b65ac4f5c3b8cbe8"
      },
      {
        "commit": "56db3a18909a029042f7ceded2e19d96aec57cdc",
        "path": "vm/vm.cpp",
        "sha256": "2ed93572891bc9e32f7525270c7ff6a88edb3d5f22529e3f19195f71cb1ff998"
      },
      {
        "commit": "56db3a18909a029042f7ceded2e19d96aec57cdc",
        "path": "ternaryos/kernel/kernel_abi.hpp",
        "sha256": "23bd9c8d1a6529fb4d46d8f910325f40ebd7b94ef40a0b0cced01eb3722f2903"
      }
    ],
    "local_links_checked": 12,
    "adr_sha256": "5c176eb6766f81cdb6ae1a0716d3f6ab60bab2c9a0377478ab6bfad53b591e2d",
    "changed_files": [
      "docs/architecture/adr/ADR-0007-axion-ai-001-governance-broker.md",
      "docs/records/audits/2026-11-17-axion-ai-001-adr-001/VALIDATION.md"
    ],
    "limits": [
      "Text/link/hash checks do not establish behavioral correctness or reviewer independence.",
      "Mermaid source inspected; diagram rendering not executed.",
      "PortableTaskState and ABI are typed schema outlines, not active machine schemas.",
      "No runtime, models, fault injection, benchmarks, qualification or host probes executed."
    ]
  }
}
```

## Reproduction of the preparation-only check

Save this source outside the repository and run it with `python3 -B` from the
candidate checkout, changing only `root` to that checkout. It reads Markdown and
Git objects and does not execute any proposed architecture or acceptance case.

```python
import hashlib,json,re,subprocess,sys
from pathlib import Path
root=Path('/private/tmp/t81-axion-ai-001-adr')
base='56db3a18909a029042f7ceded2e19d96aec57cdc'
adr=root/'docs/architecture/adr/ADR-0007-axion-ai-001-governance-broker.md'
text=adr.read_text(); checks={}
def check(name,condition):
 if not condition:raise ValueError(name)
 checks[name]='PASS'
def git(*args):return subprocess.check_output(['git',*args],cwd=root)
check('baseline commit',git('rev-parse',base).decode().strip()==base and subprocess.run(['git','merge-base','--is-ancestor',base,'HEAD'],cwd=root).returncode==0)
check('architecture header and maturity',all(t in text for t in ['Status: Proposed','Last Updated: 2026-11-17','Maturity: Experimental','Version: 0.1.0']))
required=['Purpose','Scope','Context: implemented vs experimental boundary','Decision','Alternatives Considered','Key invariants and Acceptance Criteria','Failure modes and consequences','Indeterminate: six outstanding human decision gates','Evidence and references','Versioning statement and independent-review handoff']
check('ADR and architecture-standard sections',all('## '+s in text for s in required))
rows=re.findall(r'^\| (I\d{2}) ([^\n]+)$',text,re.M)
check('all I01-I13 rows exactly once',[i for i,_ in rows]==[f'I{i:02}' for i in range(1,14)])
check('each invariant has counterexample and acceptance/falsifier cells',all(len(r.split('|'))==4 and all(c.strip() for c in r.split('|')[:3]) for _,r in rows))
check('six human gates exactly once',re.findall(r'\*\*(HD\d{2}):',text)==[f'HD{i:02}' for i in range(1,7)])
check('classification vocabulary',all(x in text for x in ['IMPLEMENTED','STATICALLY VERIFIED','DYNAMICALLY VERIFIED','PROPOSED','UNSUPPORTED']))
check('proposal and no-execution boundary',all(x in text for x in ['NON-NORMATIVE','No F01 receipt meaning','NOT EXECUTED','not normatively adopt','No exactly-once remote execution claim']))
check('dispatch and recovery terminology',all(x in text for x in ['INTENT_PENDING','INTENT_DURABLE','HANDOFF_ARMED','HANDOFF_CONFIRMED','NO_HANDOFF_PROVEN','partially persisted','UNCERTAIN','Session evidence is INCOMPLETE']))
check('all original effect outcome states',all(re.search(r'^\| '+x+r' \|',text,re.M) for x in ['NOT_DISPATCHED','DISPATCHED','CONFIRMED','DENIED','FAILED','UNCERTAIN']))
check('context-shrinkage and provenance clauses',all(x in text for x in ['ContextBudgetExceeded','reserved\n+output'.replace('\n+','\n'),'StateTransformationRef','ContextAdmissionRef','exact-fit and one-over','separately authorized, provenance-preserving compaction','cross-model token IDs, KV\ncaches'.replace('cross-model','Cross-model')]))
check('F01 holds and protocol digest retained',all(x in text for x in ['BOLQ NOT ADOPTED','residual risks NOT ACCEPTED','M1–M6\nSTOP','linux_relock_required=true','d48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3']))
refs=dict(re.findall(r'^\[([^\]]+)\]: (\S+)$',text,re.M))
for label in ['E01','E02','E03','E04','E05','E06','validation']:check('reference definition '+label,label in refs)
used=re.findall(r'(?<!!)\[[^\]\n]+\]\[([^\]\n]+)\]',text)
check('reference-style labels resolve',all(x in refs for x in used))
links=re.findall(r'(?<!!)\[[^\]\n]+\]\(([^)]+)\)',text)+list(refs.values())
pins=[];local_count=0
for link in links:
 if link.startswith('https://github.com/t81dev/t81-foundation/blob/'):
  m=re.fullmatch(r'https://github.com/t81dev/t81-foundation/blob/([0-9a-f]{40})/([^#]+)(?:#L(\d+))?',link);check('immutable source URL '+link,bool(m))
  rev,path,line=m.groups();check('source pin '+path,rev==base);b=git('show',rev+':'+path)
  if line:check('source line anchor '+path,0<int(line)<=len(b.splitlines()))
  pins.append({'commit':rev,'path':path,'sha256':hashlib.sha256(b).hexdigest()})
 elif '://' in link:raise ValueError('unexpected external link '+link)
 else:
  local_count+=1;check('local link '+link,(adr.parent/link.split('#')[0]).is_file())
check('balanced Markdown fences',sum(l.startswith('```') for l in text.splitlines())%2==0)
check('closed D3 schema unchanged',json.loads(git('show',base+':spec/rfcs/RFC-00D3-model-manifest-schema.json'))['additionalProperties'] is False)
changed=git('diff','--name-only',base).decode().splitlines()+git('ls-files','--others','--exclude-standard').decode().splitlines()
expected={'docs/architecture/adr/ADR-0007-axion-ai-001-governance-broker.md','docs/records/audits/2026-11-17-axion-ai-001-adr-001/VALIDATION.md'}
check('exact two-file documentation-only scope',set(changed)==expected and len(changed)==2)
check('no baseline tracked files modified',git('diff','--name-only','--diff-filter=MDRT',base).strip()==b'')
report={'base':base,'python':sys.version,'working_directory':str(root),'checks':checks,'pinned_sources':pins,'local_links_checked':local_count,'adr_sha256':hashlib.sha256(adr.read_bytes()).hexdigest(),'changed_files':sorted(changed),'limits':['Text/link/hash checks do not establish behavioral correctness or reviewer independence.','Mermaid source inspected; diagram rendering not executed.','PortableTaskState and ABI are typed schema outlines, not active machine schemas.','No runtime, models, fault injection, benchmarks, qualification or host probes executed.']}
Path('/private/tmp/t81-axion-adr-static-results.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'PASS: {len(checks)} static documentation checks; {len(pins)} immutable source identities; {local_count} local links; I01-I13 and HD01-HD06 present; exact two-file scope')
```
