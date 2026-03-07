#!/bin/bash
# Cleanup script for docs directory
cd /Users/t81dev/Code/t81-foundation/docs

# Remove temporary and summary files
rm -f PHASE3_MODEL_LOADING_AND_MEMORY_MANAGEMENT.md
rm -f README_LINK_VERIFICATION.md
rm -f REORGANIZATION_SUMMARY.md
rm -f STATUS_CONTENT_UPDATES_SUMMARY.md
rm -f STATUS_FILES_AUDIT_SUMMARY.md
rm -f STATUS_FILES_REFRESH_SUMMARY.md
rm -f STATUS_SUBSTANTIVE_UPDATES_SUMMARY.md
rm -f T81_FOUNDATION_PROJECT_PROFILE.md
rm -f .DS_Store
rm -rf tests

echo "Docs cleanup completed"
