#!/bin/bash
# Security Scanner Script
# Runs Trivy security scan and generates/updates security reports

set -e  # Stop on error

# === Configuration ===
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TRIVY_BIN="$SCRIPT_DIR/bin/trivy"
REPORT_DIR="reports"
SECURITY_REPORT="$REPORT_DIR/SECURITY_REPORT.md"
JSON_REPORT="$REPORT_DIR/security-scan.json"
CPP_JSON_REPORT="$REPORT_DIR/cpp-dependencies-scan.json"
SYSTEM_JSON_REPORT="$REPORT_DIR/system-packages-scan.json"
HTML_REPORT="$REPORT_DIR/security-report.html"

# === Ensure Trivy is installed ===
if [ ! -f "$TRIVY_BIN" ]; then
    echo "🔧 Installing Trivy..."
    mkdir -p ./bin
    curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b ./bin
fi

# === Create reports directory ===
mkdir -p "$REPORT_DIR"

echo "🔒 Starting Security Scan..."
echo "📅 Scan Date: $(date)"
echo "📍 Directory: $(pwd)"

# === Run comprehensive security scan ===
echo "🔍 Running filesystem vulnerability scan..."
"$TRIVY_BIN" fs . --format table --skip-files unittests/cpp/common/CommonTestHelpers.h --ignorefile .trivyignore

echo ""
echo "🔍 Scanning C++ build dependencies..."
if [ -d "build/cpp" ]; then
    "$TRIVY_BIN" fs build/cpp --scanners vuln --format table --skip-files unittests/cpp/common/CommonTestHelpers.h
    "$TRIVY_BIN" fs build/cpp --scanners vuln --format json --output "$CPP_JSON_REPORT" --skip-files unittests/cpp/common/CommonTestHelpers.h
fi

echo ""
echo "🔍 Scanning system packages for C++ libraries..."
"$TRIVY_BIN" rootfs /usr --scanners vuln --format table
"$TRIVY_BIN" rootfs /usr --scanners vuln --format json --output "$SYSTEM_JSON_REPORT"

echo ""
echo "🔍 Generating detailed JSON report..."
"$TRIVY_BIN" fs . --scanners vuln,secret,misconfig --format json --output "$JSON_REPORT" --skip-files unittests/cpp/common/CommonTestHelpers.h --ignorefile .trivyignore

# === Generate/Update Security Report ===
echo ""
echo "📝 Generating security report..."

# Get vulnerability counts from JSON
if [ -f "$JSON_REPORT" ]; then
    # Use Python to parse JSON since jq might not be available
    VULN_COUNTS=$(python3 -c "
import json
import sys
import os
try:
    all_findings = []
    
    # Parse main filesystem scan
    if os.path.exists('$JSON_REPORT'):
        with open('$JSON_REPORT', 'r') as f:
            data = json.load(f)
        for r in data.get('Results', []):
            if 'Vulnerabilities' in r:
                all_findings.extend(r['Vulnerabilities'])
            if 'Misconfigurations' in r:
                all_findings.extend(r['Misconfigurations'])
            if 'Secrets' in r:
                all_findings.extend(r['Secrets'])
    
    # Parse C++ build dependencies scan
    if os.path.exists('$CPP_JSON_REPORT'):
        with open('$CPP_JSON_REPORT', 'r') as f:
            data = json.load(f)
        for r in data.get('Results', []):
            if 'Vulnerabilities' in r:
                all_findings.extend(r['Vulnerabilities'])
    
    # Parse system packages scan
    if os.path.exists('$SYSTEM_JSON_REPORT'):
        with open('$SYSTEM_JSON_REPORT', 'r') as f:
            data = json.load(f)
        for r in data.get('Results', []):
            if 'Vulnerabilities' in r:
                all_findings.extend(r['Vulnerabilities'])
    
    critical = sum(1 for f in all_findings if f.get('Severity') == 'CRITICAL')
    high = sum(1 for f in all_findings if f.get('Severity') == 'HIGH') 
    medium = sum(1 for f in all_findings if f.get('Severity') == 'MEDIUM')
    low = sum(1 for f in all_findings if f.get('Severity') == 'LOW')
    
    print(f'{critical} {high} {medium} {low}')
except Exception as e:
    print('0 0 0 0')
    sys.exit(0)
" 2>/dev/null || echo "0 0 0 0")
    
    read CRITICAL_COUNT HIGH_COUNT MEDIUM_COUNT LOW_COUNT <<< "$VULN_COUNTS"
    TOTAL_VULNS=$((CRITICAL_COUNT + HIGH_COUNT + MEDIUM_COUNT + LOW_COUNT))
else
    CRITICAL_COUNT=0
    HIGH_COUNT=0
    MEDIUM_COUNT=0
    LOW_COUNT=0
    TOTAL_VULNS=0
fi

# Generate Markdown report
cat > "$SECURITY_REPORT" << EOF
# Security Assessment Report
**Project:** Catena SDK Project  
**Scan Date:** $(date "+%B %d, %Y at %H:%M %Z")  
**Scanner:** Trivy $(./bin/trivy --version | head -1 | cut -d' ' -f2)  
**Scan Directory:** $(pwd)

## Executive Summary

The security scan identified **$TOTAL_VULNS total vulnerabilities** in the project dependencies, filesystem, C++ build dependencies, and system packages.

### Scan Coverage
- 📁 **Filesystem:** Project files and configurations
- 🔧 **C++ Dependencies:** Build artifacts and CMake dependencies  
- 📦 **System Packages:** Installed C++ libraries (gRPC, Protocol Buffers, etc.)
- 🔐 **Secrets:** JWT tokens and API keys (excluding test files)

### Vulnerability Breakdown
- 🔴 **Critical:** $CRITICAL_COUNT
- 🟠 **High:** $HIGH_COUNT  
- 🔶 **Medium:** $MEDIUM_COUNT
- 🔵 **Low:** $LOW_COUNT

## Security Status Summary

| Severity | Count | Status |
|----------|-------|--------|
| 🔴 Critical | $CRITICAL_COUNT | $([ $CRITICAL_COUNT -eq 0 ] && echo "✅ Clean" || echo "⚠️ Action Required") |
| 🟠 High | $HIGH_COUNT | $([ $HIGH_COUNT -eq 0 ] && echo "✅ Clean" || echo "⚠️ Action Required") |
| 🔶 Medium | $MEDIUM_COUNT | $([ $MEDIUM_COUNT -eq 0 ] && echo "✅ Clean" || echo "⚠️ Review Needed") |
| 🔵 Low | $LOW_COUNT | $([ $LOW_COUNT -eq 0 ] && echo "✅ Clean" || echo "ℹ️ Monitor") |

## Detailed Findings

EOF

# Add detailed vulnerability information if JSON report exists
if [ -f "$JSON_REPORT" ] && [ $TOTAL_VULNS -gt 0 ]; then
    # Add vulnerability summary table
    echo "### 🚨 Vulnerability Summary Table" >> "$SECURITY_REPORT"
    echo "" >> "$SECURITY_REPORT"
    
    python3 -c "
import json
import sys
import os

try:
    all_findings = []
    
    # Process all JSON reports
    json_files = ['$JSON_REPORT', '$CPP_JSON_REPORT', '$SYSTEM_JSON_REPORT']
    
    for json_file in json_files:
        if not os.path.exists(json_file):
            continue
            
        with open(json_file, 'r') as f:
            data = json.load(f)
        
        # Collect all findings (vulnerabilities, misconfigurations, secrets)
        for result in data.get('Results', []):
            target = result.get('Target', 'N/A')
            result_type = result.get('Type', 'N/A')
            
            # Handle vulnerabilities (dependencies)
            if 'Vulnerabilities' in result and result['Vulnerabilities']:
                for vuln in result['Vulnerabilities']:
                    all_findings.append({
                        'id': vuln.get('VulnerabilityID', 'N/A'),
                        'severity': vuln.get('Severity', 'N/A'),
                        'package': vuln.get('PkgName', 'N/A'),
                        'installed': vuln.get('InstalledVersion', 'N/A'),
                        'fixed': vuln.get('FixedVersion', ''),
                        'title': vuln.get('Title', 'N/A')[:70] + ('...' if len(vuln.get('Title', '')) > 70 else ''),
                        'target': target,
                        'type': 'vulnerability'
                    })
            
            # Handle misconfigurations (Docker, etc.) - only in main report
            if json_file == '$JSON_REPORT' and 'Misconfigurations' in result and result['Misconfigurations']:
                for misconfig in result['Misconfigurations']:
                    all_findings.append({
                        'id': misconfig.get('ID', 'N/A'),
                        'severity': misconfig.get('Severity', 'N/A'),
                        'package': target.split('/')[-1] if '/' in target else target,
                        'installed': 'N/A',
                        'fixed': '🔧 Manual fix required',
                        'title': misconfig.get('Title', 'N/A')[:70] + ('...' if len(misconfig.get('Title', '')) > 70 else ''),
                        'target': target,
                        'type': 'misconfiguration'
                    })
            
            # Handle secrets - only in main report
            if json_file == '$JSON_REPORT' and 'Secrets' in result and result['Secrets']:
                for secret in result['Secrets']:
                    all_findings.append({
                        'id': secret.get('RuleID', 'SECRET'),
                        'severity': secret.get('Severity', 'MEDIUM'),
                        'package': target.split('/')[-1] if '/' in target else target,
                        'installed': 'N/A',
                        'fixed': '🔐 Remove/mask secret',
                        'title': secret.get('Title', 'Secret detected')[:70] + ('...' if len(secret.get('Title', '')) > 70 else ''),
                        'target': target,
                        'type': 'secret'
                    })
    
    # Sort by severity (Critical > High > Medium > Low)
    severity_order = {'CRITICAL': 0, 'HIGH': 1, 'MEDIUM': 2, 'LOW': 3}
    all_findings.sort(key=lambda x: severity_order.get(x['severity'], 4))
    
    # Print table header
    print('| Severity | CVE/ID | Package/File | Installed | Fixed | Issue |')
    print('|----------|--------|--------------|-----------|-------|-------|')
    
    for finding in all_findings:
        severity_icon = {
            'CRITICAL': '🔴',
            'HIGH': '🟠',
            'MEDIUM': '🔶', 
            'LOW': '🔵'
        }.get(finding['severity'], '⚪')
        
        fixed_status = finding['fixed'] if finding['fixed'] else '❌ Not fixed'
        package_info = finding['package'] if finding['package'] != 'N/A' else finding['target']
        
        print(f\"| {severity_icon} {finding['severity']} | {finding['id']} | {package_info} | {finding['installed']} | {fixed_status} | {finding['title']} |\")
    
    print()
    
except Exception as e:
    print('Error generating vulnerability table')
    sys.exit(0)
" >> "$SECURITY_REPORT" 2>/dev/null || echo "Error generating vulnerability table" >> "$SECURITY_REPORT"

    echo "### Detailed Vulnerabilities by Target" >> "$SECURITY_REPORT"
    echo "" >> "$SECURITY_REPORT"
    
    # Extract vulnerability details using Python
    python3 -c "
import json
import sys

try:
    with open('$JSON_REPORT', 'r') as f:
        data = json.load(f)
    
    for result in data.get('Results', []):
        if 'Vulnerabilities' in result and result['Vulnerabilities']:
            print(f\"**Target:** {result['Target']}  \")
            print(f\"**Type:** {result.get('Type', 'N/A')}  \")
            print(f\"**Vulnerabilities:** {len(result['Vulnerabilities'])}\")
            print()
            
            for vuln in result['Vulnerabilities']:
                vuln_id = vuln.get('VulnerabilityID', 'N/A')
                severity = vuln.get('Severity', 'N/A')
                pkg_name = vuln.get('PkgName', 'N/A')
                installed_ver = vuln.get('InstalledVersion', 'N/A')
                title = vuln.get('Title', 'N/A')
                fixed_ver = vuln.get('FixedVersion', '')
                references = vuln.get('References', [])
                reference = references[0] if references else 'N/A'
                
                print(f\"- **{vuln_id}** ({severity})  \")
                print(f\"  **Library:** {pkg_name} {installed_ver}  \")
                print(f\"  **Issue:** {title}  \")
                if fixed_ver:
                    print(f\"  **Fixed in:** {fixed_ver}  \")
                print(f\"  **Reference:** {reference}\")
                print()
            
except Exception as e:
    print('Error parsing vulnerability details')
    sys.exit(0)
" >> "$SECURITY_REPORT" 2>/dev/null || echo "Error parsing vulnerability details" >> "$SECURITY_REPORT"
else
    echo "✅ **No vulnerabilities detected in this scan.**" >> "$SECURITY_REPORT"
fi

# Add recommendations section
cat >> "$SECURITY_REPORT" << EOF

## Recommendations

### Immediate Actions
$(if [ $CRITICAL_COUNT -gt 0 ] || [ $HIGH_COUNT -gt 0 ]; then
    echo "🚨 **URGENT:** Address critical and high-severity vulnerabilities immediately"
    echo "- Review the detailed findings above"
    echo "- Update vulnerable packages to fixed versions"
    echo "- Test application functionality after updates"
fi)

$(if [ $MEDIUM_COUNT -gt 0 ]; then
    echo "⚠️ **MEDIUM PRIORITY:** Plan to address medium-severity vulnerabilities"
    echo "- Schedule updates for medium-severity issues"
    echo "- Monitor for exploit availability"
fi)

### Security Best Practices
- 🔄 Run security scans regularly (weekly/bi-weekly)
- 📦 Keep dependencies updated
- 🔍 Monitor security advisories for your tech stack
- 🛡️ Consider implementing automated security scanning in CI/CD

## Scan Details

- **Scan Type:** Filesystem vulnerability and secret scanning
- **Files Scanned:** All project files and dependencies
- **Report Location:** \`$JSON_REPORT\`
- **Last Updated:** $(date)

## Quick Commands

\`\`\`bash
# Re-run security scan
./scan_security.sh

# View detailed JSON report
cat $JSON_REPORT | python3 -m json.tool

# Check for specific vulnerability
./bin/trivy fs . --format table --skip-files unittests/cpp/common/CommonTestHelpers.h | grep -i "CVE-XXXX-XXXX"
\`\`\`

---
*Report generated automatically by Trivy security scanner*
EOF

echo "✅ Security report generated: $SECURITY_REPORT"

# === Generate HTML report for browser viewing ===
if command -v pandoc >/dev/null 2>&1; then
    echo "📄 Generating HTML report..."
    pandoc "$SECURITY_REPORT" -o "$HTML_REPORT" --standalone --css-url="https://cdn.jsdelivr.net/npm/github-markdown-css@5.2.0/github-markdown-light.css"
    echo "✅ HTML report generated: $HTML_REPORT"
fi

# === Summary ===
echo ""
echo "📊 Security Scan Summary:"
echo "   🔴 Critical: $CRITICAL_COUNT"
echo "   🟠 High: $HIGH_COUNT"
echo "   🔶 Medium: $MEDIUM_COUNT"
echo "   🔵 Low: $LOW_COUNT"
echo "   📝 Report: $SECURITY_REPORT"
echo "   📋 JSON: $JSON_REPORT"
if [ -f "$CPP_JSON_REPORT" ]; then
    echo "   🔧 C++ Deps: $CPP_JSON_REPORT"
fi
if [ -f "$SYSTEM_JSON_REPORT" ]; then
    echo "   📦 System Packages: $SYSTEM_JSON_REPORT"
fi
if [ -f "$HTML_REPORT" ]; then
    echo "   🌐 HTML: $HTML_REPORT"
fi

# === Return appropriate exit code ===
if [ $CRITICAL_COUNT -gt 0 ] || [ $HIGH_COUNT -gt 0 ]; then
    echo ""
    echo "⚠️  WARNING: Critical or high-severity vulnerabilities found!"
    echo "   Please review and address these issues immediately."
    exit 1
elif [ $MEDIUM_COUNT -gt 0 ]; then
    echo ""
    echo "ℹ️  INFO: Medium-severity vulnerabilities found."
    echo "   Consider addressing these in your next maintenance cycle."
    exit 0
else
    echo ""
    echo "✅ No critical security issues detected."
    exit 0
fi