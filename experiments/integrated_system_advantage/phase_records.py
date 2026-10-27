"""Independent accounting checks for Investigation 08 sidecars, not a runtime framework."""
from collections import defaultdict


def validate_record(record):
    if record.get('schema') != 't81.research.investigation08.phases.v1':
        raise ValueError('unexpected phase schema')
    events = record['events']
    if not events or events[0]['parent'] is not None or events[0]['start_ns'] != 0:
        raise ValueError('missing rooted application window')
    children = defaultdict(list)
    for index, event in enumerate(events):
        if event['id'] != index or event['end_ns'] < event['start_ns']:
            raise ValueError('invalid event ordering or interval')
        parent = event['parent']
        if index:
            if parent is None or not 0 <= parent < index:
                raise ValueError('invalid parent')
            outer = events[parent]
            if not outer['start_ns'] <= event['start_ns'] <= event['end_ns'] <= outer['end_ns']:
                raise ValueError('child outside parent')
            children[parent].append(event)
        inclusive = event['end_ns']-event['start_ns']
        if inclusive != event['inclusive_ns']:
            raise ValueError('inclusive value differs from timestamps')
    for index, event in enumerate(events):
        previous = event['start_ns']
        for child in children[index]:
            if child['start_ns'] < previous:
                raise ValueError('overlapping direct children')
            previous = child['end_ns']
        exclusive = event['inclusive_ns']-sum(child['inclusive_ns'] for child in children[index])
        if exclusive < 0 or event['exclusive_ns'] != exclusive:
            raise ValueError('invalid exclusive accounting')
    if sum(event['exclusive_ns'] for event in events) != events[0]['inclusive_ns']:
        raise ValueError('exclusive sum differs from application window')
    return events[0]['inclusive_ns']


def aggregate(record):
    validate_record(record)
    result = defaultdict(lambda: dict(count=0,inclusive_ns=0,exclusive_ns=0))
    for event in record['events']:
        row = result[event['name']]
        row['count'] += 1
        row['inclusive_ns'] += event['inclusive_ns']
        row['exclusive_ns'] += event['exclusive_ns']
    return dict(result)
