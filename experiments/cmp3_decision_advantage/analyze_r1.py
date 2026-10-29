"""Approved r1 analyzer. Only authenticated test-only diagnostics are enabled."""
import argparse,json
from r1_analysis import Invalid,diagnose,json_value
from runner_r1 import verify_test_bundle,verify_measurement_bundle,admit_test_diagnostic

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--bundle',required=True)
    parser.add_argument('--trust',required=True)
    parser.add_argument('--test-only',action='store_true')
    parser.add_argument('--admit-test-journal')
    args=parser.parse_args()
    try:
        if not args.test_only:
            verify_measurement_bundle(args.bundle,args.trust)
        verified=verify_test_bundle(args.bundle,args.trust)
        result=diagnose(verified['csv'])
        if args.admit_test_journal: admit_test_diagnostic(verified,args.admit_test_journal)
        print(json.dumps(json_value(result),sort_keys=True,allow_nan=False))
        return 0
    except Invalid as error:
        print(json.dumps({'measurement_outcome':'BLOCKED','reason':str(error),'measurement_authorized':False}))
        return 6
if __name__=='__main__':raise SystemExit(main())
