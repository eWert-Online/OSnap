---
id: actions
title: "Actions"
sidebar_position: 2
---

import Tabs from "@theme/Tabs";
import TabItem from "@theme/TabItem";

# Actions

As seen in the [configuration options](configuration) of a test file, you are able to run some actions, before the screenshot will be taken.

This is useful to bring the page into a specific state, you want to capture. <br />
**For example:** opening a dropdown, typing something in an input or waiting for an animation to be completed.

The currently available actions are `wait`, `click`, `type`, `scroll` and `function`. They may be configured like this.

:::info
All actions may be configured with the optional `@` key.
It configures the sizes this action is run on. <br />
If `@` is not present, the action runs on all sizes.
:::

## Action Types

### Wait

#### Options:

- `timeout`: The number of ms to wait, before executing the next action or taking the screenshot.

#### Example:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- "@": ["xxl"]
  action: "wait"
  timeout: 2000
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "@": ["xxl"],
    "action": "wait",
    "timeout": 2000
  }
]
```

</TabItem>
</Tabs>

---

### Click

#### Options:

- `selector`: A css selector of an element which should be clicked. If the selector evaluates to multiple elements, only the first one is clicked.

#### Example:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- "@": ["xxl"]
  action: "click"
  selector: "#id-to-click"
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "@": ["xxl"],
    "action": "click",
    "selector": "#id-to-click"
  }
]
```

</TabItem>
</Tabs>

---

### Type

#### Options:

- `selector`: A css selector of an element which should be typed in. If the selector evaluates to multiple elements, the first one will be used.
- `text`: The text to type into the element

#### Example:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- "@": ["xxl"]
  action: "type"
  selector: "#search"
  text: "some searchword"
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "@": ["xxl"],
    "action": "type",
    "selector": "#search",
    "text": "some searchword"
  }
]
```

</TabItem>
</Tabs>

### Scroll

#### Options:

- `selector`: A css selector of an element which should scrolled into view. If the selector evaluates to multiple elements, only the first one will be scrolled to.
- `px`: The number of px the page should be scrolled by

#### Example:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- "@": ["xxl"]
  action: "scroll"
  px: 200

- "@": ["l"]
  action: "scroll"
  selector: ".button"
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "@": ["xxl"],
    "action": "scroll",
    "px": 200
  },
  {
    "@": ["l"],
    "action": "scroll",
    "selector": ".button"
  }
]
```

</TabItem>
</Tabs>

### Function

#### Options:

- `name`: The name of the function to be executed

#### Example:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- action: function
  name: accept_cookies
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "action": "function",
    "name": "accept_cookies"
  }
]
```

</TabItem>
</Tabs>
