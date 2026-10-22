# CI Workflow Confirmation

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CI Workflow Confirmation](#ci-workflow-confirmation)
  - [October 21 enforcement and CI audit](#october-21-enforcement-and-ci-audit)
  - [AI Experiments CI Workflow](#ai-experiments-ci-workflow)
    - [Triggers](#triggers)

<!-- T81-TOC:END -->


Status: Active
Last Updated: 2026-10-21


## October 21 enforcement and CI audit

The merged PR #537 experiment found that the configured CanonFS text-policy hook
allowed its denied, revoked, and malformed-policy output cases. Historical
implementation/compliance entries below do not establish enforcement at that
boundary. Production hardening and a separately labeled arm D are under test;
no full inventory or system-wide security result is claimed.

AI Experiments CI run [39644888844](https://github.com/t81dev/t81-foundation/actions/runs/39644888844)
failed in the status-document freshness check (three documents aged eight days,
limit seven), before configuration or compilation. This failure is independent
of the opt-in experiment. These status notes record that audit without changing
freshness limits or claiming that subsequent CI stages have passed.


## AI Experiments CI Workflow

This document confirms the status of the AI Experiments CI Workflow.

### Triggers

The workflow is triggered on push and pull requests affecting relevant directories.
