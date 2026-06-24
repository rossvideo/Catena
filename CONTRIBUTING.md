# Contributing to Catena

Thank you for your interest in contributing to Catena! This guide covers everything you need to get started, whether you're an external contributor or a member of the Ross Video team.

We welcome all kinds of contributions — bug reports, feature requests, documentation improvements, and code changes.

## Table of Contents

- [Communication](#communication)
- [Reporting Issues](#reporting-issues)
- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [Making Changes](#making-changes)
- [Signed Commits](#signed-commits)
- [Testing](#testing)
- [Submitting a Pull Request](#submitting-a-pull-request)
- [Review Process](#review-process)
- [CI/CD Checks](#cicd-checks)
- [Project Structure](#project-structure)

---

## Communication

All project communication happens on GitHub:

- **Questions & discussions** — Open an issue or comment on an existing one.
- **Bug reports & feature requests** — Use the [issue template](#reporting-issues).
- **PR feedback** — Comment directly on the pull request.

---

## Reporting Issues

Use the repository's issue template when filing bugs or feature requests. Please include:

- A clear, descriptive title prefixed with the SDK type (e.g., `[C++] Crash when ...`, `[Go] Add support for ...`).
- Steps to reproduce (for bugs).
- Expected vs actual behavior.
- Relevant logs, screenshots, or device models.

---

## Getting Started

### External Contributors

1. **Fork** the repository on GitHub.
2. **Clone** your fork locally.
3. Set up the upstream remote:
   ```bash
   git remote add upstream https://github.com/rossvideo/Catena.git
   ```
4. Keep your fork in sync:
   ```bash
   git fetch upstream
   git merge upstream/develop
   ```

### Internal Contributors (Ross Video)

1. **Clone** the repository directly — no fork required.
2. Create feature branches off `develop`.

---

## Development Environment

Catena uses a Dev Container for a consistent development experience across platforms. The container includes all necessary build tools and dependencies.

### Prerequisites

- Docker
- VS Code with the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension
- WSL (Windows only)

### Setup

1. Clone the repository (or your fork).
2. Open the project in VS Code.
3. Run: `Ctrl+Shift+P` → **Dev Containers: Rebuild and Reopen in Container**

For detailed setup and build instructions, see [docs/DevContainer.md](docs/DevContainer.md).

---

## Making Changes

1. Create a branch from `develop`:
   ```bash
   git checkout develop
   git pull origin develop
   git checkout -b 1234-short-description
   ```

   Branch names follow the pattern `<issue-number>-<title>` (e.g., `42-add-alarm-support`). The easiest way is to use GitHub's **"Create a branch"** button on the issue page, which generates this automatically.

2. Make your changes, committing in logical chunks.

3. Write clear commit messages:
   ```
   Short summary (50 chars or less)

   Longer explanation if needed. Wrap at 72 characters.
   Explain what and why, not how.
   ```

---

## Signed Commits

All commits must be **signed** (GPG or SSH). This verifies authorship and ensures every commit shows as **Verified** on GitHub.

Set up commit signing by following GitHub's guide:  
👉 [Signing commits — GitHub Docs](https://docs.github.com/en/authentication/managing-commit-signature-verification/signing-commits)

Once configured, you can sign all commits by default:
```bash
git config --global commit.gpgsign true
```

Signed commits are enforced via branch protection on `develop` and `main` — pushes containing unsigned commits will be rejected.

> **Note:** Your `~/.gitconfig` is copied into the container at build time. If you change your git configuration on the host (e.g., adding a signing key), you'll need to rebuild the container to pick up the changes: `Ctrl+Shift+P` → **Dev Containers: Rebuild Container**.

---

## Testing

All contributions must include appropriate tests and must not reduce existing coverage. See the SDK-specific documentation for build and test commands.

Generally you can run tests + coverage with:
```bash
./scripts/run_coverage.sh
```

---

## Submitting a Pull Request

1. Push your branch:
   ```bash
   git push origin 1234-short-description
   ```

2. Open a Pull Request against `develop` on GitHub. Prefix the title with the SDK type: `[C++]`, `[Go]`, etc.

3. Write the PR title in **changelog style** — a short phrase starting with a past-tense verb that describes what changed:
   ```
   [C++] Fixed crash when device model contains empty params
   [Go] Added support for alarm table subscriptions
   [C++] Updated protobuf dependency to v28
   ```

   Common prefixes: `Fixed`, `Added`, `Removed`, `Updated`, `Refactored`, `Improved`.

4. Fill out the PR template completely:
   - **Description** — What does this change do and why?
   - **Definition of Done** — Check off all applicable items.
   - **Implementation Notes** — Anything reviewers should know.
   - **Related Issues** — Link any relevant issues.
   - **Risks / Open Questions** — Flag anything uncertain.

4. Ensure all CI checks pass.

5. When your PR is ready for review, add the **Ready For Review** label. This signals to maintainers and triggers automated analysis.

### PR Guidelines

- Keep PRs focused — one logical change per PR.
- Include tests for new functionality.
- Update documentation if you're changing behavior.
- Respond to review feedback promptly.
- Squash fixup commits before merge if requested.

---

## Review Process

- At least **one approval from a project maintainer** is required before merging.
- Only maintainers can merge pull requests.
- Reviewers may ask you to cover low-hanging untested paths even if overall coverage meets the threshold.
- Expect feedback within a few business days. If you haven't heard back, feel free to ping on the PR.

### Review Rounds

1. Add the **Ready For Review** label when your PR is ready.
2. Reviewers will remove the label when requesting changes.
3. After addressing feedback, re-add the label and use GitHub's **Re-request review** button to notify your reviewer.

---

## CI/CD Checks

Pull requests automatically trigger the following checks:

| Check | What it does |
|-------|--------------|
| Unit Tests | Runs the full test suite |
| Coverage | Generates and reports code coverage |
| CodeQL | Static analysis for security vulnerabilities |
| Trivy | Container vulnerability scanning |
| clang-format | Code style enforcement |

All checks must pass before a PR can be merged.

---

## Project Structure

| Path | Description |
|------|-------------|
| `smpte/interface/proto/` | Catena protobuf definitions |
| `smpte/interface/schemata/` | JSON schema for device models |
| `sdks/` | SDK source (C++, Go, Java) |
| `scripts/` | Build, formatting, and CI scripts |
| `docs/` | Project documentation |
| `unittests/` | Unit tests |
| `examples/` | Example implementations |

---

## License

By contributing to Catena, you agree that your contributions will be licensed under the project's existing license. See [LICENSE.txt](LICENSE.txt) for details.
