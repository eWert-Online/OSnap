---
id: configuration
title: "Configuration"
sidebar_position: 3
---

import Tabs from "@theme/Tabs";
import TabItem from "@theme/TabItem";

# Configuration

Before you can run your first test suite, OSnap needs to be configured. <br />
To do this, you need two files. The global config file and at least one test file.

All configuration files may be written in either `JSON` or `YAML` format.

The global config file has to be named `osnap.config.json` or `osnap.config.yaml` and lives in the root folder of your project.
If you want to have the config file in a different location, you may specify the `--config` flag as an [cli option](cli).

## Options

### Base Url

- **Key**: `baseUrl`
- **Required**: `true`
- **Type**: `string`

The base url used for all of your tests. This gets prepended to all your test urls.

---

### Default Sizes

- **Key**: `defaultSizes`
- **Required**: `true`
- **Type**: `Array<{name: string?, width: int, height: int}>`

An array of default sizes to run your tests in. If fullScreen is set to true, the height is the minimum height of the snapshot.

You may specify an optional `name` for each size. This is useful for running actions or ignore regions only on some screen sizes.

---

### Snapshot Directory

- **Key**: `snapshotDirectory`
- **Required**: `false`
- **Type**: `string`
- **Default**: `./__snapshots__`

The relative path to a folder, where OSnap will save the base images and test results in. <br />
This path is relative to the global config file.

---

### Fullscreen

- **Key**: `fullScreen`
- **Required**: `false`
- **Type**: `boolean (true / false)`
- **Default**: `true`

Set this to `true` if you want to capture a screenshot of the whole page. <br />
If it is set to `false`, only the visible part of the viewport is captured.

---

### Threshold

- **Key**: `threshold`
- **Required**: `false`
- **Type**: `int`
- **Default**: `0`

The number of pixels allowed to be different, before the test will be marked as failed.

---

### Retry

- **Key**: `retry`
- **Required**: `false`
- **Type**: `int`
- **Default**: `1`

The number of times a failed test should be retried before it is reported as failed.

---

### Parallelism (DEPRECATED)

- **Key**: `parallelism`
- **Required**: `false`
- **Type**: `int`
- **Default**: `3`

The number of workers OSnap should spawn to run your tests.

---

### Test Pattern

- **Key**: `testPattern`
- **Required**: `false`
- **Type**: `string`
- **Default**: `**/*.osnap.json`

A glob pattern used to locate the test files to run.
If you want to use `YAML` as your test format, you have to change this.

---

### Ignore Patterns

- **Key**: `ignorePatterns`
- **Required**: `false`
- **Type**: `Array<string>`
- **Default**: `["**/node_modules/**"]`

An array of glob patterns to not search for test files in.

---

### Diff-Pixel Color

- **Key**: `diffPixelColor`
- **Required**: `false`
- **Type**: `{r: int, g: int, b: int}`
- **Default**: `{"r": 255, "g": 0, "b": 0}`

The color in rgb format used to highlight different pixels in the diff image. <br />
Only values between 0 and 255 are allowed. The default color is red.

---

### Functions

- **Key**: `functions`
- **Required**: `false`
- **Type**: `Record<name, Array<action>>`
- **Default**: `{}`

A record of which the key is used as the function name and the value is an array of actions to be executed, when that function is called.

## Example

**A full example of a global config file:**

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
baseUrl: "http://localhost:3000"
fullScreen: true
threshold: 10
parallelism: 20

snapshotDirectory: "./__image-snapshots__"
testPattern: "src/**/*.osnap.yaml"
ignorePatterns:
  - node_modules
  - vendor
  - dist

diffPixelColor:
  r: 209
  g: 63
  b: 255

defaultSizes:
  - name: xxl
    width: 1600
    height: 900
  - name: xl
    width: 1366
    height: 768
  - name: md
    width: 1024
    height: 576
  - name: sm
    width: 768
    height: 432
  - name: xs
    width: 640
    height: 360
  - name: xxs
    width: 320
    height: 180

functions:
  accept_cookies:
    - action: click
      selector: "#cookie-consent .settings"
    - action: click
      selector: "#cookie-consent .accept"
    - action: click
      selector: "#cookie-consent .confirm"
  login:
    - action: type
      selector: "#username"
      text: "my_testuser"
    - action: type
      selector: "#password"
      text: "password123!"
    - action: click
      selector: "#submit_login"
```

</TabItem>
<TabItem value="json" label="JSON">

```json
{
  "baseUrl": "http://localhost:3000",
  "fullScreen": true,
  "threshold": 10,
  "parallelism": 20,
  "snapshotDirectory": "./__image-snapshots__",
  "testPattern": "src/**/*.osnap.json",
  "ignorePatterns": ["node_modules", "vendor", "dist"],
  "diffPixelColor": {
    "r": 209,
    "g": 63,
    "b": 255
  },
  "defaultSizes": [
    { "name": "xxl", "width": 1600, "height": 900 },
    { "name": "xl", "width": 1366, "height": 768 },
    { "name": "md", "width": 1024, "height": 576 },
    { "name": "sm", "width": 768, "height": 432 },
    { "name": "xs", "width": 640, "height": 360 },
    { "name": "xxs", "width": 320, "height": 180 }
  ],
  "functions": {
    "accept_cookies": [
      {
        "action": "click",
        "selector": "#cookie-consent .settings"
      },
      {
        "action": "click",
        "selector": "#cookie-consent .accept"
      },
      {
        "action": "click",
        "selector": "#cookie-consent .confirm"
      }
    ],
    "login": [
      {
        "action": "type",
        "selector": "#username",
        "text": "my_testuser"
      },
      {
        "action": "type",
        "selector": "#password",
        "text": "password123!"
      },
      {
        "action": "click",
        "selector": "#submit_login"
      }
    ]
  }
}
```

</TabItem>
</Tabs>
