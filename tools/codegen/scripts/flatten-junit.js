#!/usr/bin/env node
// Lightweight JUnit flattener for jest-junit output.
// Merges all <testsuite> elements in tools/codegen/junit.xml into a single suite.
// Intended for CI usage only; does not run Jest itself.

import fs from 'fs';
import path from 'path';

// Support running from repo root (CI) or from tools/codegen directory (local)
const candidates = [
  path.resolve(process.cwd(), 'junit.xml'),
  path.resolve(process.cwd(), 'tools/codegen/junit.xml')
];
const junitPath = candidates.find(p => fs.existsSync(p)) || candidates[0];
console.log(`[flatten-junit] Using junit path: ${junitPath}`);

function readXml(p) {
  if (!fs.existsSync(p)) {
    console.error(`[flatten-junit] File not found: ${p}`);
    process.exit(1);
  }
  return fs.readFileSync(p, 'utf8');
}

function extractSuites(xml) {
  return [...xml.matchAll(/<testsuite[\s\S]*?<\/testsuite>/g)].map(m => m[0]);
}

function extractTestcases(suiteXml) {
  return [...suiteXml.matchAll(/<testcase[\s\S]*?<\/testcase>/g)].map(m => m[0]);
}

function flatten(xml) {
  const suites = extractSuites(xml);
  if (suites.length <= 1) {
    console.log('[flatten-junit] Already single suite; no changes made.');
    return xml;
  }

  const cases = suites.flatMap(extractTestcases);

  const totalTests = cases.length;
  const totalFailures = cases.reduce((n, c) => n + (c.includes('<failure') ? 1 : 0), 0);
  const totalErrors = 0; // jest-junit encodes as failures
  const totalSkipped = cases.reduce((n, c) => n + (c.includes('<skipped') ? 1 : 0), 0);
  const time = suites.reduce((sum, s) => {
    const m = s.match(/time="([0-9.]+)"/);
    return sum + (m ? parseFloat(m[1]) : 0);
  }, 0).toFixed(3);

  const headerMatch = xml.match(/<\?xml[\s\S]*?\?>/);
  const header = headerMatch ? headerMatch[0] : '<?xml version="1.0" encoding="UTF-8"?>';
  const timestamp = new Date().toISOString();

  const merged = `${header}\n<testsuites name="catena-codegen" tests="${totalTests}" failures="${totalFailures}" errors="${totalErrors}" time="${time}">\n  <testsuite name="catena-codegen" errors="${totalErrors}" failures="${totalFailures}" skipped="${totalSkipped}" timestamp="${timestamp}" time="${time}" tests="${totalTests}">\n${cases.map(c => '    ' + c).join('\n')}\n  </testsuite>\n</testsuites>\n`;
  return merged;
}

try {
  const original = readXml(junitPath);
  const flattened = flatten(original);
  if (flattened !== original) {
    fs.writeFileSync(junitPath, flattened);
    console.log('[flatten-junit] Successfully flattened JUnit XML to single suite.');
  }
} catch (err) {
  console.error('[flatten-junit] Error flattening JUnit XML:', err);
  process.exit(1);
}
