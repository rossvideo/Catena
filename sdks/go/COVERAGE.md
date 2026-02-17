# Go Coverage with VS Code Coverage Gutters

This document explains how to view Go test coverage in VS Code using Coverage Gutters.

## Quick Start

1. **Generate coverage in LCOV format:**
   
   **Option A: Using the project script (recommended):**
   ```bash
   cd ~/Catena
   bash scripts/run_coverage.sh
   ```
   
   **Option B: Using Make (from sdks/go directory):**
   ```bash
   cd sdks/go
   make coverage-lcov
   ```

2. **Enable Coverage Gutters in VS Code:**
   - Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
   - Type "Coverage Gutters: Watch"
   - Press Enter

3. **View coverage:**
   - Open any `.go` file
   - You'll see coverage indicators in the gutter:
     - 🟢 Green: Covered lines
     - 🔴 Red: Uncovered lines
     - No indicator: Non-executable lines

## Coverage Generation Methods

### Using Project Script (Recommended)

The `scripts/run_coverage.sh` script is integrated with the project's build system:

```bash
cd ~/Catena
bash scripts/run_coverage.sh [OPTIONS]
```

**Options:**
- `--html` - Generate HTML coverage report
- `-v` or `--verbose` - Show verbose test output
- `-c` or `--clean` - Clean test cache before running

**What it generates:**
- `~/Catena/coverage/go_coverage.out` - Go coverage data
- `~/Catena/coverage/go_coverage.lcov` - LCOV format for Coverage Gutters
- `~/Catena/coverage/go_coverage.html` - HTML report (with `--html` flag)

### Using Make Targets

Alternatively, you can use Make directly from the `sdks/go` directory:

- `make test` - Run tests without coverage
- `make coverage` - Run tests and generate `coverage.out` (Go format)
- `make coverage-html` - Generate HTML coverage report (`coverage.html`)
- `make coverage-lcov` - Generate LCOV format for Coverage Gutters (`coverage.lcov`)

## Files Generated

### When using `scripts/run_coverage.sh`:
- `~/Catena/coverage/go_coverage.out` - Go's native coverage format
- `~/Catena/coverage/go_coverage.lcov` - LCOV format for Coverage Gutters (always generated)
- `~/Catena/coverage/go_coverage.html` - HTML report (with `--html` flag)

### When using Make targets:
- `sdks/go/coverage.out` - Go's native coverage format
- `sdks/go/coverage.html` - HTML report (with `make coverage-html`)
- `sdks/go/coverage.lcov` - LCOV format for Coverage Gutters (with `make coverage-lcov`)

## VS Code Settings

The following settings have been configured in `.vscode/settings.json`:

```json
{
  "coverage-gutters.coverageFileNames": [
    "sdks/go/coverage.lcov",
    "coverage.lcov",
    "lcov.info"
  ],
  "coverage-gutters.coverageBaseDir": "**",
  "coverage-gutters.showLineCoverage": true,
  "coverage-gutters.showRulerCoverage": true
}
```

## Troubleshooting

### Coverage not showing?
1. Make sure you've run `make coverage-lcov` first
2. Check that `coverage.lcov` exists in `sdks/go/`
3. Try toggling Coverage Gutters off and on again
4. Check the Output panel (View > Output) and select "Coverage Gutters" from the dropdown

### Need to install gcov2lcov manually?
The `scripts/run_coverage.sh` script automatically installs `gcov2lcov` if needed. 

If you need to install it manually:
```bash
go install github.com/jandelgado/gcov2lcov@latest
```

Note: The Make target `make coverage-lcov` also checks and installs `gcov2lcov` automatically.

## Additional Resources

- [Coverage Gutters Extension](https://marketplace.visualstudio.com/items?itemName=ryanluker.vscode-coverage-gutters)
- [Go Testing Documentation](https://go.dev/blog/cover)
