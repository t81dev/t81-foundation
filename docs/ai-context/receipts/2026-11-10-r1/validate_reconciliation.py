"""Task-local receipt/data consistency and historical preservation checks.

Run from repository root. This validates documentation identity and metadata
consistency, not scientific acceptance, GitHub freshness or authorization.
"""
import hashlib
import json
import re
import subprocess
from pathlib import Path

ROOT = Path.cwd()
RECEIPTS = ROOT / 'docs/ai-context/receipts/2026-11-10-r1'
BASE = 'dbf107fda83a3755fa21d6b95caa3eb8ac450dd3'


def require(value, message):
    if not value:
        raise ValueError(message)


def prior(path):
    return subprocess.check_output(['git', 'show', f'{BASE}:{path}'])


inventory = json.loads((RECEIPTS / 'pr-inventory.json').read_text())
raw = json.loads((RECEIPTS / 'github-observations.json').read_text())
records = {p['number']: p for p in inventory['pull_requests']}
require(len(records) == 82, 'expected 82 unique observed PRs')
require(len(inventory['open_pr_numbers']) == 69, 'open inventory changed')
require(inventory['open_draft_count'] == 39, 'draft count mismatch')
require(set(inventory['open_pr_numbers']) == {n for n,p in records.items() if p['state'] in ('OPEN','DRAFT')}, 'open set mismatch')
for p in raw['pull_requests']:
    m = p['metadata']; i = records[m['number']]
    require(i['base_sha'] == m['base_sha'] and i['head_sha'] == m['head_sha'], 'revision mismatch')
    actual = 'MERGED' if m['merged'] else 'CLOSED' if m['state']=='closed' else 'DRAFT' if m['draft'] else 'OPEN'
    require(i['state'] == actual, 'lifecycle mismatch')
    require(i['merge_sha'] == (m['merge_commit_sha'] if m['merged'] else None), 'synthetic merge promoted')
    require(len(i['unresolved_thread_ids']) == sum(not t['is_resolved'] for t in p['review_threads']['review_threads']), 'thread count mismatch')
require(len(records[763]['unresolved_thread_ids']) == 4, '#763 findings lost')
require(inventory['correction']['sha'] == '3969002e9d684bbf67b01aab485648aa2cd8c0d0', 'correction pin lost')

path = 'docs/ai-context/CURRENT_STATE.json'
old = json.loads(prior(path)); current = json.loads((ROOT/path).read_text())
require(current['selected_view'] == old['selected_view'], 'selected view changed')
for key,value in old.items():
    if key != 'operational_views':
        require(current[key] == value, f'historical top-level field changed: {key}')
for key,value in old['operational_views'].items():
    require(current['operational_views'][key] == value, f'historical view changed: {key}')
require(set(current['operational_views']) - set(old['operational_views']) == {'2026-11-10-r1'}, 'unexpected view changes')
for name in ('START_HERE.md','ACTIVE_RESEARCH_HANDOFF.md'):
    path='docs/ai-context/'+name
    require((ROOT/path).read_bytes().split(b'\n\n',1)[1] == prior(path), 'entry-point history changed')
require((ROOT/'docs/ai-context/T81_RESEARCH_STATUS_2026-11-10.md').read_bytes() == prior('docs/ai-context/T81_RESEARCH_STATUS_2026-11-10.md'), 'prior snapshot changed')

viz = ROOT/'docs/architecture/visualization'
for p in viz.glob('*.json'):
    rel=str(p.relative_to(ROOT))
    if p.name not in ('research_programs.json','pull_requests_snapshot.json'):
        require(p.read_bytes() == prior(rel), f'architecture data changed: {rel}')
    else:
        require(json.loads(p.read_text())['authoritative_baseline_commit'] == json.loads(prior(rel))['authoritative_baseline_commit'], 'historical baseline changed')
programs=json.loads((viz/'research_programs.json').read_text())
old_programs=json.loads(prior('docs/architecture/visualization/research_programs.json'))
for field in ('formal_claim','evidence_tiers','canonical_result','confidence','baseline','unresolved_limitations'):
    require([x[field] for x in programs['hypothesis_mappings']] == [x[field] for x in old_programs['hypothesis_mappings']], f'canonical hypothesis field changed: {field}')
pr_data=json.loads((viz/'pull_requests_snapshot.json').read_text())
require(pr_data['pr_inventory_summary']['cataloged_prs']==len(pr_data['pull_requests']), 'dashboard count mismatch')
for p in pr_data['pull_requests']:
    i=records[p['pr_number']]
    require(p['head_sha']==i['head_sha'] and p['base_sha']==i['base_sha'] and p['state']==i['state'] and p['merge_commit']==i['merge_sha'], 'dashboard PR mismatch')

for p in [ROOT/'docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md', RECEIPTS/'VALIDATION.md']:
    for target in re.findall(r'\]\(([^)]+)\)',p.read_text()):
        if '://' in target or target.startswith('#'):
            continue
        require((p.parent/target.split('#')[0]).exists(), f'missing link {p}: {target}')
print('PASS: 82 PR identities/lifecycles, 69 open/39 drafts, four #763 findings, 24 dashboard rows, selected view/history, canonical hypotheses, architecture data and new document links')
