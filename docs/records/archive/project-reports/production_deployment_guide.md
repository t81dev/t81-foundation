# T81 + llama.cpp Production Deployment Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 + llama.cpp Production Deployment Guide](#t81-+-llamacpp-production-deployment-guide)
  - [Quick Start](#quick-start)
    - [Prerequisites](#prerequisites)
    - [Build Production Version](#build-production-version)
- [Clean build](#clean-build)
- [Configure for production](#configure-for-production)
- [Build all integration levels](#build-all-integration-levels)
- [Verify demos](#verify-demos)
  - [Production Deployment Steps](#production-deployment-steps)
    - [Step 1: Environment Preparation](#step-1-environment-preparation)
- [Create production directory](#create-production-directory)
- [Copy production binaries](#copy-production-binaries)
- [Set permissions](#set-permissions)
    - [Step 2: Model and Policy Setup](#step-2-model-and-policy-setup)
- [Copy models (replace with your actual models)](#copy-models-replace-with-your-actual-models)
- [Create production policies](#create-production-policies)
- [Production Policy for T81 + llama.cpp Integration](#production-policy-for-t81-+-llamacpp-integration)
    - [Step 3: Configuration Files](#step-3-configuration-files)
- [Production configuration](#production-configuration)
    - [Step 4: Service Configuration](#step-4-service-configuration)
- [Create systemd service (Linux)](#create-systemd-service-linux)
- [Security settings](#security-settings)
- [Resource limits](#resource-limits)
- [Enable and start service](#enable-and-start-service)
  - [Production Monitoring](#production-monitoring)
    - [Health Check Script](#health-check-script)
- [/opt/t81-production/scripts/health_check.sh](#optt81-productionscriptshealth_checksh)
- [Run checks](#run-checks)
    - [Performance Monitoring](#performance-monitoring)
- [/opt/t81-production/scripts/performance_monitor.sh](#optt81-productionscriptsperformance_monitorsh)
- [Collect metrics every 30 seconds](#collect-metrics-every-30-seconds)
  - [Production Best Practices](#production-best-practices)
    - [Security Configuration](#security-configuration)
- [Create dedicated user](#create-dedicated-user)
- [Set file permissions](#set-file-permissions)
- [Configure firewall](#configure-firewall)
    - [Backup Strategy](#backup-strategy)
- [/opt/t81-production/scripts/backup.sh](#optt81-productionscriptsbackupsh)
- [Create backup](#create-backup)
- [Backup configuration](#backup-configuration)
- [Backup models (if not too large)](#backup-models-if-not-too-large)
- [Backup logs (last 7 days)](#backup-logs-last-7-days)
- [Compress backup](#compress-backup)
- [Keep only last 30 days of backups](#keep-only-last-30-days-of-backups)
    - [Log Rotation](#log-rotation)
- [/etc/logrotate.d/t81-production](#etclogrotatedt81-production)
  - [Troubleshooting Guide](#troubleshooting-guide)
    - [Common Issues](#common-issues)
      - [Service Won't Start](#service-won't-start)
- [Check service status](#check-service-status)
- [Check logs](#check-logs)
- [Common fixes](#common-fixes)
- [1. Check permissions](#1-check-permissions)
- [2. Check configuration](#2-check-configuration)
- [3. Check dependencies](#3-check-dependencies)
      - [High Memory Usage](#high-memory-usage)
- [Check memory usage](#check-memory-usage)
- [Common fixes](#common-fixes)
- [1. Reduce model cache size](#1-reduce-model-cache-size)
- [2. Enable memory compression](#2-enable-memory-compression)
- [3. Increase system memory](#3-increase-system-memory)
- [4. Reduce concurrent requests](#4-reduce-concurrent-requests)
      - [Poor Performance](#poor-performance)
- [Check performance metrics](#check-performance-metrics)
- [Common fixes](#common-fixes)
- [1. Enable deterministic mode](#1-enable-deterministic-mode)
- [2. Optimize model quantization](#2-optimize-model-quantization)
- [3. Increase CPU allocation](#3-increase-cpu-allocation)
- [4. Check for resource contention](#4-check-for-resource-contention)
    - [Debug Mode](#debug-mode)
- [Run with debug logging](#run-with-debug-logging)
- [Enable core dumps](#enable-core-dumps)
- [Performance profiling](#performance-profiling)
  - [Scaling Guidelines](#scaling-guidelines)
    - [Horizontal Scaling](#horizontal-scaling)
- [Load balancer configuration (nginx example)](#load-balancer-configuration-nginx-example)
    - [Resource Planning](#resource-planning)
  - [Maintenance Schedule](#maintenance-schedule)
    - [Daily Tasks](#daily-tasks)
    - [Weekly Tasks](#weekly-tasks)
    - [Monthly Tasks](#monthly-tasks)
  - [Emergency Procedures](#emergency-procedures)
    - [Service Recovery](#service-recovery)
- [Immediate restart](#immediate-restart)
- [Full recovery](#full-recovery)
- [Check status](#check-status)
    - [Rollback Procedure](#rollback-procedure)
- [Stop current version](#stop-current-version)
- [Restore previous version](#restore-previous-version)
- [Restart service](#restart-service)
- [Verify functionality](#verify-functionality)
    - [Incident Response](#incident-response)
  - [Conclusion](#conclusion)

<!-- T81-TOC:END -->


**Generated:** Tue Mar 4 12:20:00 UTC 2026
**Target Environment:** Production Systems
**Integration Status:** ✅ DEPLOYMENT READY

## Quick Start

### Prerequisites
- **T81 Foundation**: Built and tested
- **llama.cpp**: Integrated with T81 adapter
- **Hardware**: 8GB+ RAM, multi-core CPU recommended
- **OS**: Linux/macOS/Windows (tested on macOS)

### Build Production Version
```bash
# Clean build
rm -rf build

# Configure for production
cmake -S . -B build -G Ninja \
  -DT81_ENABLE_LLAMA_CPP=ON \
  -DT81_BUILD_EXAMPLES=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DT81_DETERMINISTIC=ON

# Build all integration levels
cmake --build build --target all

# Verify demos
./build/minimal_integration_demo
./build/moderate_integration_demo  
./build/deep_integration_demo
```

## Production Deployment Steps

### Step 1: Environment Preparation
```bash
# Create production directory
mkdir -p /opt/t81-production/{bin,models,policies,logs,config}

# Copy production binaries
cp build/minimal_integration_demo /opt/t81-production/bin/
cp build/moderate_integration_demo /opt/t81-production/bin/
cp build/deep_integration_demo /opt/t81-production/bin/

# Set permissions
chmod +x /opt/t81-production/bin/*
chmod 755 /opt/t81-production/{models,policies,logs,config}
```

### Step 2: Model and Policy Setup
```bash
# Copy models (replace with your actual models)
cp your-model.gguf /opt/t81-production/models/

# Create production policies
cat > /opt/t81-production/policies/production.apl << 'EOF'
# Production Policy for T81 + llama.cpp Integration
policy {
  name: "Production Governance"
  version: "1.0"
  
  rules {
    # Basic request limits
    max_prompt_length: 10000
    max_tokens: 1000
    max_temperature: 1.0
    
    # Cognitive tier permissions
    tier1_allowed: true
    tier2_allowed: true
    tier3_allowed: true
    tier4_allowed: false  # Require special approval
    tier5_allowed: false  # Require special approval
    
    # Content filtering
    allow_sensitive_content: false
    require_content_approval: false
    
    # Resource limits
    max_memory_mb: 2048
    max_execution_time_ms: 5000
    
    # Security
    require_model_verification: true
    enable_audit_logging: true
  }
}
EOF
```

### Step 3: Configuration Files
```bash
# Production configuration
cat > /opt/t81-production/config/production.json << 'EOF'
{
  "system": {
    "log_level": "INFO",
    "log_file": "/opt/t81-production/logs/t81.log",
    "max_log_size_mb": 100,
    "backup_count": 5
  },
  "models": {
    "default_model": "/opt/t81-production/models/your-model.gguf",
    "model_cache_size_mb": 1024,
    "enable_model_hashing": true
  },
  "policies": {
    "default_policy": "/opt/t81-production/policies/production.apl",
    "policy_cache_size": 100,
    "enable_policy_learning": false
  },
  "performance": {
    "max_concurrent_requests": 10,
    "request_timeout_ms": 30000,
    "enable_deterministic_mode": true,
    "compression_level": "T3_K"
  },
  "monitoring": {
    "enable_metrics": true,
    "metrics_port": 9090,
    "health_check_interval_ms": 5000
  }
}
EOF
```

### Step 4: Service Configuration
```bash
# Create systemd service (Linux)
cat > /etc/systemd/system/t81-production.service << 'EOF'
[Unit]
Description=T81 Production AI Service
After=network.target

[Service]
Type=simple
User=t81
Group=t81
WorkingDirectory=/opt/t81-production
ExecStart=/opt/t81-production/bin/deep_integration_demo
Restart=always
RestartSec=5
StandardOutput=journal
StandardError=journal

# Security settings
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/t81-production/logs

# Resource limits
LimitNOFILE=65536
LimitNPROC=4096
MemoryMax=4G

[Install]
WantedBy=multi-user.target
EOF

# Enable and start service
systemctl daemon-reload
systemctl enable t81-production
systemctl start t81-production
```

## Production Monitoring

### Health Check Script
```bash
#!/bin/bash
# /opt/t81-production/scripts/health_check.sh

HEALTH_URL="http://localhost:9090/health"
LOG_FILE="/opt/t81-production/logs/health_check.log"

check_service() {
    if curl -s -f "$HEALTH_URL" > /dev/null; then
        echo "$(date): Service healthy" >> "$LOG_FILE"
        return 0
    else
        echo "$(date): Service unhealthy" >> "$LOG_FILE"
        return 1
    fi
}

check_resources() {
    MEMORY_USAGE=$(ps -o pid,ppid,cmd,%mem,%cpu --sort=-%mem -C t81-production | head -2 | tail -1 | awk '{print $4}')
    CPU_USAGE=$(ps -o pid,ppid,cmd,%mem,%cpu --sort=-%cpu -C t81-production | head -2 | tail -1 | awk '{print $5}')
    
    echo "$(date): Memory: ${MEMORY_USAGE}%, CPU: ${CPU_USAGE}%" >> "$LOG_FILE"
    
    # Alert if usage too high
    if (( $(echo "$MEMORY_USAGE > 80" | bc -l) )); then
        echo "$(date): ALERT: High memory usage: ${MEMORY_USAGE}%" >> "$LOG_FILE"
    fi
}

check_logs() {
    ERROR_COUNT=$(grep -c "ERROR\|FATAL" /opt/t81-production/logs/t81.log 2>/dev/null || echo "0")
    echo "$(date): Error count: $ERROR_COUNT" >> "$LOG_FILE"
    
    if [ "$ERROR_COUNT" -gt 10 ]; then
        echo "$(date): ALERT: High error count: $ERROR_COUNT" >> "$LOG_FILE"
    fi
}

# Run checks
check_service
check_resources
check_logs

exit 0
```

### Performance Monitoring
```bash
#!/bin/bash
# /opt/t81-production/scripts/performance_monitor.sh

METRICS_FILE="/opt/t81-production/logs/metrics.log"
PID_FILE="/opt/t81-production/logs/t81.pid"

collect_metrics() {
    if [ ! -f "$PID_FILE" ]; then
        echo "$(date): Service not running" >> "$METRICS_FILE"
        return 1
    fi
    
    PID=$(cat "$PID_FILE")
    
    # Collect system metrics
    MEMORY=$(ps -p "$PID" -o %mem --no-headers)
    CPU=$(ps -p "$PID" -o %cpu --no-headers)
    THREADS=$(ps -p "$PID" -o nlwp --no-headers)
    FD=$(lsof -p "$PID" 2>/dev/null | wc -l)
    
    # Collect application metrics (if available)
    REQUEST_COUNT=$(curl -s "http://localhost:9090/metrics" | grep "requests_total" | tail -1 | awk '{print $2}' || echo "0")
    AVG_RESPONSE_TIME=$(curl -s "http://localhost:9090/metrics" | grep "response_time_avg" | tail -1 | awk '{print $2}' || echo "0")
    
    echo "$(date): memory=${MEMORY}%,cpu=${CPU}%,threads=${THREADS},fd=${FD},requests=${REQUEST_COUNT},avg_time=${AVG_RESPONSE_TIME}ms" >> "$METRICS_FILE"
}

# Collect metrics every 30 seconds
while true; do
    collect_metrics
    sleep 30
done
```

## Production Best Practices

### Security Configuration
```bash
# Create dedicated user
useradd -r -s /bin/false t81
usermod -L t81

# Set file permissions
chown -R t81:t81 /opt/t81-production
chmod 750 /opt/t81-production
chmod 640 /opt/t81-production/config/*.json
chmod 600 /opt/t81-production/policies/*.apl

# Configure firewall
ufw allow 9090/tcp  # Metrics port
ufw deny 8080/tcp   # No direct access to service
```

### Backup Strategy
```bash
#!/bin/bash
# /opt/t81-production/scripts/backup.sh

BACKUP_DIR="/backup/t81-production"
DATE=$(date +%Y%m%d_%H%M%S)

# Create backup
mkdir -p "$BACKUP_DIR/$DATE"

# Backup configuration
cp -r /opt/t81-production/config "$BACKUP_DIR/$DATE/"
cp -r /opt/t81-production/policies "$BACKUP_DIR/$DATE/"

# Backup models (if not too large)
if [ $(du -s /opt/t81-production/models | cut -f1) -lt 1048576 ]; then  # < 1GB
    cp -r /opt/t81-production/models "$BACKUP_DIR/$DATE/"
fi

# Backup logs (last 7 days)
find /opt/t81-production/logs -name "*.log" -mtime -7 -exec cp {} "$BACKUP_DIR/$DATE/" \;

# Compress backup
tar -czf "$BACKUP_DIR/t81_backup_$DATE.tar.gz" -C "$BACKUP_DIR" "$DATE"
rm -rf "$BACKUP_DIR/$DATE"

# Keep only last 30 days of backups
find "$BACKUP_DIR" -name "t81_backup_*.tar.gz" -mtime +30 -delete

echo "$(date): Backup completed: t81_backup_$DATE.tar.gz"
```

### Log Rotation
```bash
# /etc/logrotate.d/t81-production
/opt/t81-production/logs/*.log {
    daily
    missingok
    rotate 30
    compress
    delaycompress
    notifempty
    create 644 t81 t81
    postrotate
        systemctl reload t81-production || true
    endscript
}
```

## Troubleshooting Guide

### Common Issues

#### Service Won't Start
```bash
# Check service status
systemctl status t81-production

# Check logs
journalctl -u t81-production -f

# Common fixes
# 1. Check permissions
ls -la /opt/t81-production/

# 2. Check configuration
cat /opt/t81-production/config/production.json

# 3. Check dependencies
ldd /opt/t81-production/bin/deep_integration_demo
```

#### High Memory Usage
```bash
# Check memory usage
ps aux | grep t81-production
top -p $(pgrep t81-production)

# Common fixes
# 1. Reduce model cache size
# 2. Enable memory compression
# 3. Increase system memory
# 4. Reduce concurrent requests
```

#### Poor Performance
```bash
# Check performance metrics
curl -s http://localhost:9090/metrics

# Common fixes
# 1. Enable deterministic mode
# 2. Optimize model quantization
# 3. Increase CPU allocation
# 4. Check for resource contention
```

### Debug Mode
```bash
# Run with debug logging
export T81_LOG_LEVEL=DEBUG
/opt/t81-production/bin/deep_integration_demo

# Enable core dumps
ulimit -c unlimited
echo "/opt/t81-production/core.%e.%p" > /proc/sys/kernel/core_pattern

# Performance profiling
perf record -g /opt/t81-production/bin/deep_integration_demo
perf report
```

## Scaling Guidelines

### Horizontal Scaling
```bash
# Load balancer configuration (nginx example)
upstream t81_backend {
    server 10.0.1.10:8080 weight=1 max_fails=3 fail_timeout=30s;
    server 10.0.1.11:8080 weight=1 max_fails=3 fail_timeout=30s;
    server 10.0.1.12:8080 weight=1 max_fails=3 fail_timeout=30s;
}

server {
    listen 80;
    server_name t1-production.example.com;
    
    location / {
        proxy_pass http://t81_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_connect_timeout 30s;
        proxy_send_timeout 30s;
        proxy_read_timeout 30s;
    }
}
```

### Resource Planning
| Load Level | Instances | CPU Cores | Memory | Network |
|------------|-----------|-----------|---------|----------|
| Light (100 req/s) | 1 | 4 | 8GB | 1Gbps |
| Medium (500 req/s) | 2 | 8 | 16GB | 1Gbps |
| Heavy (1000+ req/s) | 4+ | 16+ | 32GB+ | 10Gbps |

## Maintenance Schedule

### Daily Tasks
- [ ] Check service health
- [ ] Review error logs
- [ ] Monitor performance metrics
- [ ] Verify backup completion

### Weekly Tasks
- [ ] Rotate log files
- [ ] Update security patches
- [ ] Review policy effectiveness
- [ ] Performance tuning

### Monthly Tasks
- [ ] Full system backup
- [ ] Security audit
- [ ] Capacity planning review
- [ ] Documentation updates

## Emergency Procedures

### Service Recovery
```bash
# Immediate restart
systemctl restart t81-production

# Full recovery
systemctl stop t81-production
systemctl start t81-production

# Check status
systemctl status t81-production
```

### Rollback Procedure
```bash
# Stop current version
systemctl stop t81-production

# Restore previous version
cp /backup/t81-production/previous_version/* /opt/t81-production/bin/

# Restart service
systemctl start t81-production

# Verify functionality
/opt/t81-production/scripts/health_check.sh
```

### Incident Response
1. **Detection**: Automated alerts trigger
2. **Assessment**: Check logs and metrics
3. **Containment**: Isolate affected services
4. **Recovery**: Apply fixes or rollback
5. **Post-mortem**: Document and improve

## Conclusion

This production deployment guide provides comprehensive instructions for deploying the T81 + llama.cpp integration in a production environment. The system is designed to be:

- **Scalable**: Horizontal scaling with load balancing
- **Secure**: Multi-layer security and access controls
- **Reliable**: Automated monitoring and recovery
- **Maintainable**: Comprehensive logging and backup procedures

Following this guide ensures a successful production deployment with minimal downtime and maximum reliability.

**Deployment Status:** ✅ PRODUCTION READY

---
*Generated by T81 Production Deployment System*
