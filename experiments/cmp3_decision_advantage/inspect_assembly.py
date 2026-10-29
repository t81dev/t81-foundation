"""Provisional assembly/basic-block and object inventories; never PMU estimates."""
import argparse
import json
from pathlib import Path
import re
import subprocess

INSTRUCTION = re.compile(r'^\s+([a-z][a-z0-9.]*)(?:\s+(.*))?$')
LABEL = re.compile(r'^([A-Za-z_.$][\w.$]*):')
TERMINATOR = re.compile(r'^(?:ret\w*|br|b(?:\.[a-z]+)?|cbz|cbnz|tbz|tbnz|j[a-z]+)(?:\s|$)')

def inventory(text):
    # Only branch destinations form block boundaries. Mach-O Lloh labels mark
    # linker optimization hints inside straight-line code, not basic blocks.
    destinations=set()
    for raw in text.splitlines():
        line=raw.split('#',1)[0].split(';',1)[0].strip()
        if TERMINATOR.match(line) and len(line.split(None,1))==2:
            target=line.split(None,1)[1].split(',')[-1].strip().split()[0]
            if re.fullmatch(r'[A-Za-z_.$][\w.$]*',target):destinations.add(target)
    result=[];function=None;block=None;last_label='unknown';serial=0
    def begin(label):
        nonlocal block
        block={'label':label,'instructions':[]}
        function['blocks'].append(block)
    for raw in text.splitlines():
        label=LABEL.match(raw)
        if label:
            label_name=label[1]
            if not label_name.startswith('.'):
                last_label=label_name
            if function is not None and label_name in destinations:
                if block and not block['instructions']: block['label']=label_name
                else: begin(label_name)
            continue
        if raw.strip().startswith('.globl') or raw.strip().startswith('.globl\t'):
            parts=raw.strip().split()
            if len(parts)>=2 and not parts[1].startswith('.'): last_label=parts[1]
        if raw.strip().startswith('.cfi_startproc'):
            function={'symbol':last_label,'blocks':[]};begin('entry');continue
        if raw.strip().startswith('.cfi_endproc'):
            if function is None: raise ValueError('CFI_END_WITHOUT_START')
            function['blocks']=[b for b in function['blocks'] if b['instructions']]
            for b in function['blocks']: b['count']=len(b['instructions'])
            function['total']=sum(b['count'] for b in function['blocks'])
            result.append(function);function=None;block=None;continue
        if function is None:continue
        if '%bb.' in raw:
            if block and block['instructions']:begin(raw.strip())
            continue
        if raw.strip().startswith(('.inst ','.byte ','.long ')):
            raise ValueError('EMITTED_OPCODE_DIRECTIVE_REQUIRES_OBJECT_REVIEW')
        # ARM immediates use '#'; only ';' comments are stripped here.
        line=raw.split(';',1)[0].rstrip()
        match=INSTRUCTION.match(line)
        if match is None:continue
        mnemonic=match[1]
        if block is None:
            serial+=1;begin('fallthrough_'+str(serial))
        block['instructions'].append(line.strip())
        if TERMINATOR.match(line.strip()):block=None
    if function is not None:raise ValueError('UNTERMINATED_CFI_FUNCTION')
    return result

def object_inventory(path,objdump):
    run=subprocess.run([objdump,'--disassemble','--no-show-raw-insn',str(path)],text=True,capture_output=True,check=True)
    functions=[];current=None
    for line in run.stdout.splitlines():
        label=re.match(r'^\s*[0-9a-f]+\s+<([^>]+)>:',line)
        if label:
            current={'symbol':label[1],'instructions':[]};functions.append(current)
        else:
            op=re.match(r'^\s*([0-9a-f]+):\s+([a-z][a-z0-9.]*(?:\s+.*)?)$',line)
            if op and current is not None:current['instructions'].append({'offset':op[1],'instruction':op[2]})
    for function in functions:function['span_count']=len(function['instructions'])
    return {'functions':functions,'raw_disassembly':run.stdout,
            'scope':'Object spans may include alignment padding; no dynamic-count inference.'}

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('directory',type=Path)
    parser.add_argument('--objdump')
    args=parser.parse_args()
    report={}
    for name in ('candidate','baseline1','baseline2','baseline3','baseline4','payload'):
        functions=inventory((args.directory/(name+'.s')).read_text())
        if not functions:raise ValueError('NO_FUNCTION_INVENTORY')
        report[name]={'assembly':functions}
        if args.objdump:report[name]['object']=object_inventory(args.directory/(name+'.o'),args.objdump)
    def ac(name):
        matches=[f for f in report[name]['assembly'] if 'probe_' not in f['symbol'] and '_policy' not in f['symbol']]
        if len(matches)!=1:raise ValueError('AMBIGUOUS_AC_SYMBOL')
        return [i for b in matches[0]['blocks'] for i in b['instructions']]
    report['AC_candidate_baseline2_identical']=ac('candidate')==ac('baseline2')
    report['interpretation']='Static source/object lowering only, never measured instruction counts or ternary advantage.'
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
