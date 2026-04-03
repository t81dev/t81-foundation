#!/usr/bin/env bash
set -euo pipefail

echo "🧹 Cleaning up junk files before commit..."

# Remove experimental Python files at root
echo "🗑️  Removing experimental Python files..."
rm -f simple_performance_demo.py
rm -f real_performance_measurement.py  
rm -f go_to_market_execution.py
rm -f global_expansion_strategy.py

# Remove experimental directories
echo "🗑️  Removing experimental directories..."
rm -rf t81-advanced
rm -rf t81-ecosystem
rm -rf t81-enterprise
rm -rf t81-global
rm -rf t81-gtm
rm -rf t81-performance
rm -rf t81-production

# Remove experimental docs
echo "🗑️  Removing experimental docs..."
rm -rf docs/advanced
rm -rf docs/deployment
rm -rf docs/ecosystem
rm -rf docs/enterprise
rm -rf docs/global
rm -rf docs/gtm

# Remove experimental scripts
echo "🗑️  Removing experimental scripts..."
rm -f scripts/analyze_bundle_creation.py
rm -f scripts/collect_production_metrics.sh
rm -f scripts/deploy_advanced_integration.sh
rm -f scripts/deploy_ecosystem_expansion.sh
rm -f scripts/deploy_enterprise_features.sh
rm -f scripts/deploy_local_production.sh
rm -f scripts/deploy_performance_optimization.sh
rm -f scripts/deploy_production.sh
rm -f scripts/validate_production_ecosystem.sh

# Remove experimental tools
echo "🗑️  Removing experimental tools..."
rm -f tools/bundle_directory.py
rm -f tools/bundle_inspector.py
rm -f tools/bundle_monitor.py
rm -f tools/bundle_optimizer.py
rm -f tools/ide_integration.py
rm -f tools/ide_integration_clean.py
rm -f tools/simple_bundle_directory.py

# Remove experimental examples
echo "🗑️  Removing experimental examples..."
rm -rf examples/go

# Remove bundle validation files (these seem experimental)
echo "🗑️  Removing experimental bundle files..."
rm -f bundle_conformance_results.json
rm -f bundle_index.json
rm -f bundle_metrics.json

# Remove other experimental files
echo "🗑️  Removing other experimental files..."
rm -f .github/workflows/bundle-validation.yml

echo "✅ Cleanup complete!"
echo ""
echo "📊 Remaining files ready for commit:"
git status --porcelain | grep -E "^\?\?" | wc -l | xargs echo "  Untracked files:"
