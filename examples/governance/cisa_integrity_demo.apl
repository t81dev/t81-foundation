; cisa_integrity_demo.apl
;
; Axion policy for the CISA Critical Infrastructure Integrity Demo.
;
; This policy enforces hard resource limits on the T81VM execution
; so the triage program cannot spin indefinitely, consume unbounded
; stack space, or exceed its declared instruction budget — regardless
; of input. Every enforcement decision is recorded to the immutable
; CanonFS audit log.
;
; Attach at runtime:
;   t81 vm run cisa_demo.tisc --policy cisa_integrity_demo.apl
;
; Inspect the audit record after a run:
;   t81 axion log --json
;   t81 canonfs snapshot --json

(policy
  (name "CISAIntegrityDemo")

  ; Tier 1 — symbolic / deterministic computation.
  ; No external I/O, network, or filesystem access permitted.
  (tier 1)

  ; Hard instruction budget.  A triage loop over 10 readings compiles
  ; to well under 2 000 instructions; 5 000 gives comfortable headroom
  ; while still bounding any accidental infinite loop.
  (max-instructions 5000)

  ; Stack depth cap.  The demo uses shallow call depth (main → classify
  ; → verdict_label); 128 frames is generous and safe.
  (max-stack 128)

  ; Force an Axion audit event on every run so the execution is always
  ; visible in the CanonFS journal — even if the program exits normally.
  (require-axion-event (reason "CISA demo governance checkpoint"))

  ; Emit a full deterministic instruction trace for post-run audit.
  (log-level deterministic-trace)
)
