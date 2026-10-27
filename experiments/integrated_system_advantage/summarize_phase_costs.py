#!/usr/bin/env python3
"""Recompute descriptive phase costs from the raw Investigation 08 packet."""
import argparse
import json
import statistics
from phase_records import aggregate


def stats(values):
    median=statistics.median(values)
    mad=statistics.median(abs(value-median) for value in values)
    return dict(n=len(values),mean_ns=statistics.mean(values),median_ns=median,
                sample_stddev_ns=statistics.stdev(values) if len(values)>1 else None,
                min_ns=min(values),max_ns=max(values),raw_ns=values,
                descriptive_tail_indices=[i for i,value in enumerate(values) if mad and abs(value-median)>3*mad],
                tail_rule='post-collection descriptive 3*MAD flag only; no samples excluded or formal outlier inference')


def summarize(packet):
    if not packet['status'].startswith('BOUNDED NONINTERFERENCE PASSED'):
        raise ValueError('failed gate/measurement packet')
    rows=packet['measurement']['observations']
    profiles=[aggregate(row['arms']['ON']['execution']['phase_record']) for row in rows]
    names=sorted(set().union(*(profile.keys() for profile in profiles)))
    phase={name:stats([profile.get(name,{}).get('exclusive_ns',0) for profile in profiles]) for name in names}
    app=[row['arms']['ON']['execution']['phase_record']['events'][0]['inclusive_ns'] for row in rows]
    for index,window in enumerate(app):
        if sum(phase[name]['raw_ns'][index] for name in names)!=window:
            raise ValueError('phase partition does not equal app window')
    wall={arm:stats([row['arms'][arm]['execution']['wall_ns'] for row in rows]) for arm in ('OFF','ON','conventional')}
    overhead=stats([row['arms']['ON']['execution']['wall_ns']-row['arms']['OFF']['execution']['wall_ns'] for row in rows])
    boundary=stats([row['arms']['ON']['execution']['boundary_residual_ns'] for row in rows])
    warm=next(row for row in packet['measurement']['warm_native_api'] if row['arm']=='ON')
    intervals=[event['inclusive_ns'] for event in warm['record']['phase_record']['events'] if event['name']=='warm.native_probe_iteration']
    return dict(phase_exclusive_ns=phase,application_window_ns=stats(app),wall_ns=wall,
                combined_process_boundary_residual_ns=boundary,paired_instrumentation_wall_difference_ns=overhead,
                warm_native_probe_intervals_ns=intervals,
                warm_note='First versus later repetitions reuse a loaded model but repeat compilation/VM creation; no isolated OFF per-call clocks.',
                interpretation='Exploratory descriptive implementation costs; no architectural advantage/disadvantage or native-hardware inference.')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('packet')
    args=parser.parse_args()
    with open(args.packet) as stream:packet=json.load(stream)
    print(json.dumps(summarize(packet),indent=2))
