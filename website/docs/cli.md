---
title: "CLI Flags"
---

# CLI Flags

The following cli flags are currently available and should be used mainly in ci environments.

| Flag          | Description                                                                                                                                       |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| `--help`      | Show the help page of OSnap, explaining the cli useage.                                                                                           |
| `--config`    | The relative path to the global config file. (optional)                                                                                           |
| `--no-create` | With this option enabled, new snapshots will not be created, but fail the whole test run instead. This option is recommended for ci environments. |
| `--no-only`   | With this option enabled, the test run will fail, if you have any test with "only" set to true. This option is recommended for ci environments.   |
| `--no-skip`   | With this option enabled, the test run will fail, if you have any test with "skip" set to true. This option is recommended for ci environments.   |
