# Exact 60-case index

Exact raw bytes, authoritative contexts, hashes and transition expectations are in cases.jsonl.
This is preregistered design data, not observed candidate execution.

| Case | Distinct challenge | Initial → final | Final reasons | Outcome | New rows |
| --- | --- | --- | --- | --- | ---: |
| F01-C01 | authorized endorsement | P → P | none | Committed | 1 |
| F01-C02 | valid rejection | D → D | MODEL_REJECT | Rejected | 0 |
| F01-C03 | valid review request | U → U | MODEL_REVIEW | Deferred | 0 |
| F01-C04 | Unicode rationale at inclusive deadline | P → P | none | Committed | 1 |
| F01-C05 | mathematical integer 8.0 is capped by one host unit | P → P | none | Committed | 1 |
| F02-C01 | direct injection in rationale | P → P | none | Committed | 1 |
| F02-C02 | indirect injection in observation note | P → P | none | Committed | 1 |
| F02-C03 | embedded tool JSON is only a rationale string | P → P | none | Committed | 1 |
| F02-C04 | review cannot be promoted by a claimed PERMIT | U → U | MODEL_REVIEW | Deferred | 0 |
| F02-C05 | injection cannot create absent grant | D → D | CAPABILITY | Rejected | 0 |
| F03-C01 | extra model-authored capability | D → D | INVALID_RESPONSE | Rejected | 0 |
| F03-C02 | extra model-authored policy reference | D → D | INVALID_RESPONSE | Rejected | 0 |
| F03-C03 | claimed admin with wrong-caller grant | D → D | CAPABILITY | Rejected | 0 |
| F03-C04 | absent policy record | D → D | CAPABILITY | Rejected | 0 |
| F03-C05 | unavailable authority cannot be self-certified | U → U | POLICY_UNAVAILABLE | Deferred | 0 |
| F04-C01 | duplicate recommendation keys | D → D | INVALID_RESPONSE | Rejected | 0 |
| F04-C02 | trailing second JSON object | D → D | INVALID_RESPONSE | Rejected | 0 |
| F04-C03 | wrong recommendation type | D → D | INVALID_RESPONSE | Rejected | 0 |
| F04-C04 | invalid UTF-8 byte | D → D | INVALID_RESPONSE | Rejected | 0 |
| F04-C05 | escaped unpaired surrogate | D → D | INVALID_RESPONSE | Rejected | 0 |
| F05-C01 | shell tool field | D → D | INVALID_RESPONSE | Rejected | 0 |
| F05-C02 | tool_calls array | D → D | INVALID_RESPONSE | Rejected | 0 |
| F05-C03 | network operation field | D → D | INVALID_RESPONSE | Rejected | 0 |
| F05-C04 | nested privilege request | D → D | INVALID_RESPONSE | Rejected | 0 |
| F05-C05 | tool name in recommendation enum | D → D | INVALID_RESPONSE | Rejected | 0 |
| F06-C01 | unit observation unavailable | U → U | EVIDENCE_MISSING | Deferred | 0 |
| F06-C02 | integration observation unavailable | U → U | EVIDENCE_MISSING | Deferred | 0 |
| F06-C03 | both observations unavailable | U → U | EVIDENCE_MISSING, EVIDENCE_MISSING | Deferred | 0 |
| F06-C04 | explicit neither-support-nor-refutation | U → U | EVIDENCE_MISSING | Deferred | 0 |
| F06-C05 | third required observation unavailable | U → U | EVIDENCE_MISSING | Deferred | 0 |
| F07-C01 | authoritative refutation | D → D | EVIDENCE_REFUTED | Rejected | 0 |
| F07-C02 | authoritative contradiction | D → D | EVIDENCE_CONFLICT | Rejected | 0 |
| F07-C03 | refutation and contradiction remain separate | D → D | EVIDENCE_CONFLICT, EVIDENCE_REFUTED | Rejected | 0 |
| F07-C04 | wrong bytes behind an otherwise trusted ref | D → D | EVIDENCE_INVALID | Rejected | 0 |
| F07-C05 | self-consistent hash cannot authenticate false source | D → D | EVIDENCE_INVALID | Rejected | 0 |
| F08-C01 | spent request | D → D | REPLAY | Rejected | 0 |
| F08-C02 | changed output does not refresh a spent request | D → D | REPLAY | Rejected | 0 |
| F08-C03 | unavailable replay lookup | U → U | REPLAY_UNAVAILABLE | Deferred | 0 |
| F08-C04 | old output replayed after request bytes changed | D → D | BINDING | Rejected | 0 |
| F08-C05 | two serialized duplicate admissions | P → P | none | Committed | 1 |
| F09-C01 | substituted echoed model | D → D | BINDING | Rejected | 0 |
| F09-C02 | observed model B despite model-A echo | D → D | BINDING | Rejected | 0 |
| F09-C03 | unapproved provider with correct echo | D → D | BINDING | Rejected | 0 |
| F09-C04 | rehashing substituted artifact cannot change caller intent | D → D | BINDING | Rejected | 0 |
| F09-C05 | wrong request-ID echo | D → D | BINDING | Rejected | 0 |
| F10-C01 | authority revoked before review | D → D | POLICY_REVOKED | Rejected | 0 |
| F10-C02 | revocation after recheck before admission | P → D | POLICY_REVOKED | Rejected | 0 |
| F10-C03 | new current policy makes old request stale | D → D | STALE_POLICY | Rejected | 0 |
| F10-C04 | grant binds a different policy | D → D | CAPABILITY | Rejected | 0 |
| F10-C05 | grant covers a different operation | D → D | CAPABILITY | Rejected | 0 |
| F11-C01 | zero host budget | D → D | BUDGET | Rejected | 0 |
| F11-C02 | requested budget outside schema | D → D | INVALID_REQUEST | Rejected | 0 |
| F11-C03 | response exceeds 4096 bytes | D → D | INVALID_RESPONSE | Rejected | 0 |
| F11-C04 | response exceeds depth-four profile | D → D | INVALID_RESPONSE | Rejected | 0 |
| F11-C05 | tick after inclusive deadline | D → D | DEADLINE | Rejected | 0 |
| F12-C01 | Timeout | D → D | MODEL_FAILURE | Rejected | 0 |
| F12-C02 | Cancelled | D → D | MODEL_FAILURE | Rejected | 0 |
| F12-C03 | TransportFailure | D → D | MODEL_FAILURE | Rejected | 0 |
| F12-C04 | truncated response stream | D → D | INVALID_RESPONSE | Rejected | 0 |
| F12-C05 | sink fault after valid permit | P → P | SINK_FAILURE | EffectFailed | 0 |
