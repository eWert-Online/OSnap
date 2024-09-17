---
id: configuration
title: "Configuration"
sidebar_position: 1
---

import Tabs from "@theme/Tabs";
import TabItem from "@theme/TabItem";

# Configuration

The test config files (`**.osnap.json` by default), are used to specify a single test to be executed on all [`defaultSizes`](../Setup/configuration#default-sizes). A test file consists of an array of tests with the following possible options:

## Options

### Name

- **Key**: `name`
- **Required**: `true`
- **Type**: `string`

The name of the test. <br />
This has to be **unique**, as it will be used to name the base image of this snapshot.

---

### Url

- **Key**: `url`
- **Required**: `true`
- **Type**: `string`

The url (concatenated with the [`baseUrl`](../Setup/configuration#base-url)) to screenshot.

---

### Only

- **Key**: `only`
- **Required**: `false`
- **Type**: `boolean (true / false)`
- **Default**: `false`

If set to `true`, OSnap will only run tests with `only` set to true and skip all other tests. <br />
If OSnap runs with the `--no-only` cli flag, having this set to `true` will fail the test run.

---

### Skip

- **Key**: `skip`
- **Required**: `false`
- **Type**: `boolean (true / false)`
- **Default**: `false`

If set to `true`, this test will be skipped. <br />
If OSnap runs with the `--no-skip` cli flag, having this set to `true` will fail the test run.

---

### Threshold

- **Key**: `threshold`
- **Required**: `false`
- **Type**: `int`
- **Default**: _Whatever is specified in the [global threshold](../Setup/configuration#threshold)_

The number of pixels allowed to be different, before the test will be marked as failed.

---

### Retry

- **Key**: `retry`
- **Required**: `false`
- **Type**: `int`
- **Default**: _Whatever is specified in the [global retry](../Setup/configuration#retry)_

The number of times a failed test should be retried before it is reported as failed.

---

### Ignore Regions

- **Key**: `ignore`
- **Required**: `false`
- **Type**: `Array<region>`
- **Default**: `[]`

An array of regions to ignore in your screenshot. <br />
For possible options and more info refer to: **[ignore regions](ignore-regions)**.

---

### Actions

- **Key**: `actions`
- **Required**: `false`
- **Type**: `Array<action>`
- **Default**: `[]`

An array of actions to run before the screenshot is taken. <br />
For possible options and more info refer to: **[actions](actions)**.

## Example

The smallest possible test file would look like this:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- name: Login
  url: "/path/to/login.html"
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "name": "Login",
    "url": "/path/to/login.html"
  }
]
```

</TabItem>
</Tabs>

**A full example of a test file:**

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- name: Home
  url: "/"
  threshold: 20

  sizes:
    - name: xl
      width: 1600
      height: 768
    - name: xs
      width: 768
      height: 1024
    - name: xxs
      width: 320
      height: 500

  ignore:
    - "@": ["xxs"]
      x1: 0
      y1: 0
      x2: 100
      y2: 100
    - selector: ".my-animation"

  actions:
    - action: type
      selector: "#search"
      text: some text to type
    - action: click
      selector: "#id .class"
    - "@": ["xxs", "xs"]
      action: wait
      timeout: 2000
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "name": "Home",
    "url": "/",
    "threshold": 20,
    "sizes": [
      {
        "name": "xl",
        "width": 1600,
        "height": 768
      },
      {
        "name": "xs",
        "width": 768,
        "height": 1024
      },
      {
        "name": "xxs",
        "width": 320,
        "height": 500
      }
    ],
    "ignore": [
      {
        "@": ["xxs"],
        "x1": 0,
        "y1": 0,
        "x2": 100,
        "y2": 100
      },
      {
        "selector": ".my-animation"
      }
    ],
    "actions": [
      {
        "action": "type",
        "selector": "#search",
        "text": "some text to type"
      },
      {
        "action": "click",
        "selector": "#id .class"
      },
      {
        "@": ["xxs", "xs"],
        "action": "wait",
        "timeout": 2000
      }
    ]
  }
]
```

</TabItem>
</Tabs>
