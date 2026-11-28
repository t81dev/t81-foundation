package t81.h07

import rego.v1

default allow := false

# Allow only when:
# 1. Action is explicitly permitted
# 2. Target path is confined inside root (no traversal)
# 3. Artifact digest is in the allowlist
# 4. Three-Plane transition is reconciled (no direct stochastic -> deterministic bypass)
allow if {
	input.action in {"gate_check", "commit_artifact"}
	startswith(input.path, input.root)
	not contains(input.path, "..")
	input.artifact_sha in data.allowed_hashes
	plane_allowed
}

plane_allowed if {
	input.source_plane == "deterministic"
}

plane_allowed if {
	input.source_plane == "hybrid"
	input.hybrid_reconciled == true
}

deny_reason := "ERR_OUT_OF_ROOT" if {
	not startswith(input.path, input.root)
} else := "ERR_OUT_OF_ROOT" if {
	contains(input.path, "..")
} else := "ERR_PLANE_BYPASS_DENIED" if {
	input.source_plane == "stochastic"
	input.target_plane == "deterministic"
	input.hybrid_reconciled == false
} else := "ERR_DIGEST_NOT_ALLOWED" if {
	not (input.artifact_sha in data.allowed_hashes)
} else := "ALLOW" if {
	allow
} else := "ERR_POLICY_DENIED"
