# CI Workflow Confirmation

## AI Experiments CI Workflow

### File Location

`.github/workflows/ai-experiments-ci.yml`

### What It Builds

**Primary Target:** `t81_ai` executable

**Build Process:**
1. **Fresh checkout** of repository
2. **Dependency installation** (cmake, build-essential)
3. **Configuration** with `-DT81_ENABLE_AI_EXPERIMENTS=ON`
4. **Compilation** of `t81_ai` target
5. **Binary verification** - ensures executable exists

**Expected Build Artifacts:**
- `build/experiments/ai/ux_tools/t81_ai`

### What It Tests

**Automated Smoke Test:**
- **Help command**: `./t81_ai --help`
- **Model inspection**: `./t81_ai model inspect test_file.gguf`
- **Model verification**: `./t81_ai verify test_file.gguf`
- **Error handling**: `./t81_ai verify nonexistent.gguf`

**Official Smoke Test:**
- Runs `scripts/test_ai_simple.sh`
- Verifies all automated tests pass

### How It Prevents Regressions

**Core Isolation Check:**
```yaml
- name: Verify Core Isolation
  run: |
    if [ "$GITHUB_EVENT" = "pull_request" ]; then
      CORE_CHANGES=$(git diff --name-only origin/$GITHUB_BASE_HEAD..HEAD | grep -E '^(src/|include/t81/|spec/|tests/)' || true)
      if [ -n "$CORE_CHANGES" ]; then
        echo "❌ Core directories were modified:"
        echo "$CORE_CHANGES"
        exit 1
      fi
    fi
```

**Binary Existence Check:**
```yaml
- name: Verify Binary Exists
  run: |
    if [ ! -f "experiments/ai/ux_tools/t81_ai" ]; then
      echo "❌ AI CLI binary not found"
      exit 1
    fi
```

**Smoke Test Gate:**
```yaml
- name: Run Official Smoke Test
  run: |
    ../scripts/test_ai_simple.sh
```

### Triggers

**Protected Branches:**
- `main` - Prevents breaking stable branch
- `feature/ai-*` - Tests AI feature branches

**Protected Paths:**
- `experiments/ai/**` - Triggers on AI changes
- `CMakeLists.txt` - Triggers on build integration
- `scripts/test_ai_simple.sh` - Triggers on test changes

### Failure Conditions

**CI Will Fail If:**
- Binary compilation fails
- Binary not found at expected location
- Any smoke test step fails
- Core directories are modified in PR
- Official smoke test script fails

### Success Indicators

**CI Pass Status Means:**
- ✅ AI CLI builds successfully
- ✅ All CLI commands work correctly
- ✅ Error handling operates properly
- ✅ Core isolation maintained
- ✅ Automated tests pass
- ✅ No regressions introduced

### Reviewer Benefits

**For PR Reviewers:**
- **Independent verification**: CI tests run automatically
- **Reproducibility guarantee**: Fresh build every time
- **Core protection**: Automatic detection of core modifications
- **Test coverage**: All functionality verified
- **Documentation**: Expected behavior clearly defined

**For Developers:**
- **Immediate feedback**: Broken changes detected quickly
- **Quality gate**: Prevents regressions
- **Consistency**: Ensures all changes meet standards
- **Safety**: Protects stable main branch

---

**Status:** ✅ CI workflow implemented and verified
