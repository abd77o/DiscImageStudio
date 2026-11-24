# Security policy

## Scope

DiscImageStudio issues **raw SCSI/MMC write commands to an optical drive from an elevated process**. That makes
the following classes of bug security-relevant, and they should be reported privately rather than in a public
issue:

- A defect that lets the writer target a device other than the optical drive the user selected
- Path handling that lets an attacker-controlled configuration, image path or `config.ini` redirect a write,
  execute code, or escape the unpacked binary's directory
- Command-injection or memory-safety bugs reachable from an untrusted image file (`BMP`, `PNG`, `JPEG` parsing)
- Anything that removes or bypasses the *test write* lockout before a real burn
- Misuse of elevation (for example a privilege escalation caused by how the manifest or the file writers work)

Burning the wrong disc, corrupting media, or a failed burn are **expected risks of the tool**, not security
vulnerabilities — they are documented in [docs/HARDWARE_NOTES.md](docs/HARDWARE_NOTES.md).

## Supported versions

The project is pre-1.0 and experimental; only the latest `main` is supported with fixes.

## Reporting a vulnerability

Please **do not open a public issue**. Use GitHub's private vulnerability reporting
(*Security → Report a vulnerability*) on this repository. If that is unavailable, contact a maintainer directly
and say only that you have a security report — no details in public.

Include:

- the affected version or commit,
- a minimal reproduction (image file, `config.ini`, steps),
- the impact you believe it has,
- whether you are willing to be credited.

You can expect an initial response within about a week. Confirmed issues are fixed on `main` and described in
[CHANGELOG.md](CHANGELOG.md) once a fix is available.

## Safe testing

Do not test memory-safety findings against real media: the burner path can permanently destroy a disc. Prefer the
mock device in `tests/MockDevice.h` and the headless tools.
