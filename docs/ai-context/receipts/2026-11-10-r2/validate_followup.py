"""Validate focused #765 follow-up and preserve the immutable R1 observation.

Run from repository root. Metadata consistency and content identity do not
establish scientific acceptance, live GitHub state or authentic provenance.
"""
import os
import json
import re
import subprocess
from pathlib import Path

ROOT = Path.cwd()
R1 = '48dc1435e1753b426258eeb2bb14dfd036ad686b'
BASE = '36b3849aa1e6297e88de2ec1655777793ab0b59e'
RECEIPTS = ROOT / 'docs/ai-context/receipts/2026-11-10-r2'


def require(value, message):
    if not value:
        raise ValueError(message)


def prior(revision, path):
    return subprocess.check_output(['git', 'show', f'{revision}:{path}'])


def read(path):
    return json.loads((ROOT/path).read_text())


# Original snapshot and every retained R1 receipt are immutable.
paths = subprocess.check_output(['git','ls-tree','-r','--name-only',R1,
    'docs/ai-context/receipts/2026-11-10-r1',
    'docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md'], text=True).splitlines()
for path in paths:
    require((ROOT/path).read_bytes() == prior(R1,path), f'R1 bytes changed: {path}')
inventory=read('docs/ai-context/receipts/2026-11-10-r1/pr-inventory.json')
require(len(inventory['pull_requests']) == 82, 'R1 82-record inventory changed')
require(inventory['open_pr_count']==69, 'historical open count changed')

path='docs/ai-context/CURRENT_STATE.json'; old=json.loads(prior(R1,path)); current=read(path)
for key,value in old.items():
    if key=='operational_views':
        for name,view in value.items():
            require(current[key][name]==view, f'historical view changed: {name}')
    else:
        require(current[key]==value, f'historical state field changed: {key}')
require(set(current['operational_views'])-set(old['operational_views'])=={'2026-11-10-r2'}, 'unexpected operational view changes')
for name in ('START_HERE.md','ACTIVE_RESEARCH_HANDOFF.md'):
    path='docs/ai-context/'+name
    require((ROOT/path).read_bytes().split(b'\n\n',1)[1]==prior(R1,path), f'historical pointer bytes changed: {path}')

receipt=read('docs/ai-context/receipts/2026-11-10-r2/github-observations.json'); pr=receipt['pr765']
require(pr['merged'] is True and pr['merge_commit_sha']==BASE, '#765 merge identity mismatch')
require(pr['head_sha']=='2dd3cc62e526942cdcbed6d100770c08c26ab2fe', '#765 actual head mismatch')
threads=receipt['review_threads']['review_threads']
require(len(threads)==2 and all(not x['is_resolved'] for x in threads), '#765 findings lost')
require(receipt['open_count']==len(receipt['open_prs'])==70, 'follow-up open count mismatch')
require(receipt['draft_count']==sum(x['draft'] for x in receipt['open_prs'])==40, 'follow-up draft count mismatch')
subprocess.run(['git','merge-base','--is-ancestor',R1,'HEAD'],check=True)
subprocess.run(['git','merge-base','--is-ancestor',BASE,'HEAD'],check=True)
subprocess.run(['git','diff','--quiet',BASE,'--','experiments','scripts','include','spec'],check=True)

viz='docs/architecture/visualization/'
for path in subprocess.check_output(['git','ls-tree','-r','--name-only',BASE,viz],text=True).splitlines():
    if path in (viz+'research_programs.json',viz+'pull_requests_snapshot.json'):
        require(read(path)['authoritative_baseline_commit']==json.loads(prior(R1,path))['authoritative_baseline_commit'], 'historical architecture baseline changed')
    else:
        actual = os.readlink(ROOT/path).encode() if (ROOT/path).is_symlink() else (ROOT/path).read_bytes()
        require(actual==prior(BASE,path), f'architecture/frontend/coverage changed: {path}')
programs=read(viz+'research_programs.json'); old_programs=json.loads(prior(R1,viz+'research_programs.json'))
for a,b in zip(programs['hypothesis_mappings'],old_programs['hypothesis_mappings']):
    require({k:v for k,v in a.items() if k!='linked_studies'}=={k:v for k,v in b.items() if k!='linked_studies'}, 'canonical hypotheses changed')
snapshot=read(viz+'pull_requests_snapshot.json'); old_snapshot=json.loads(prior(R1,viz+'pull_requests_snapshot.json'))
require([x for x in snapshot['pull_requests'] if x['pr_number']!=765]==old_snapshot['pull_requests'], 'R1 dashboard rows changed')
new=next(x for x in snapshot['pull_requests'] if x['pr_number']==765)
require(new['state']=='MERGED' and new['merge_commit']==BASE and new['head_sha']==pr['head_sha'], 'dashboard lifecycle mismatch')
require(snapshot['pr_inventory_summary']['cataloged_prs']==len(snapshot['pull_requests'])==25, 'dashboard count mismatch')
for path in [ROOT/'docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R2.md',RECEIPTS/'VALIDATION.md']:
    for target in re.findall(r'\]\(([^)]+)\)',path.read_text()):
        if '://' not in target and not target.startswith('#'):
            require((path.parent/target.split('#')[0]).exists(), f'missing local link {target}')
for path in (ROOT/'docs/ai-context').rglob('*.json'):
    json.loads(path.read_text())
print(f'PASS: {len(paths)} original R1 snapshot/receipt files byte-identical; 82-record inventory, historical state/selection, pointers and 24 R1 dashboard rows preserved; #765 merge/two findings, 70-open/40-draft metadata, 25 mapped rows, architecture baselines/topology/frontend/coverage, canonical hypotheses and local links verified.')
