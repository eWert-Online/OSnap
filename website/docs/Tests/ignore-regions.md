---
id: ignore-regions
title: "Ignore Regions"
sidebar_position: 3
---

import Tabs from "@theme/Tabs";
import TabItem from "@theme/TabItem";

# Ignore Regions

Sometimes you might want to ignore specific regions, because you know, they will always be different. <br />
It is also possible, to ignore multiple regions, by specifying multiple coordinates and / or selectors.

:::info
All ignore regions may be configured with the optional `@` key.
It configures the sizes this ignore region is applied on. <br />
If `@` is not present, the ignore region will be applied on all sizes.
:::

## Mathods

You may either ignore a region by specifying its coordinates `(x1, y1)` & `(x2, y2)` or ignore a specific element by its selector.

### Ignoring by Coordinates

#### Options:

- `x1`: The first part of the top-left point constructing the ignore-region.
- `y1`: The second part of the top-left point constructing the ignore-region.
- `x2`: The first part of the bottom-right point constructing the ignore-region.
- `y2`: The second part of the bottom-right point constructing the ignore-region.

#### Example:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- "@": ["xxs", "xs"]
  x1: 0
  y1: 0
  x2: 100
  y2: 100
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "@": ["xxs", "xs"],
    "x1": 0,
    "y1": 0,
    "x2": 100,
    "y2": 100
  }
]
```

</TabItem>
</Tabs>

The given coordinates are used to create a rectangle.
The point specified by `(x1, y1)` is the top-left and `(x2, y2)` is the bottom-right corner of the rectangle.
Everything inside these coordinates is ignored.

---

### Ignoring by Selector

#### Options:

- `selector`: A css selector of an element which should be ignored. If the selector evaluates to multiple elements, only the first one is ignored.

#### Example:

<Tabs>
<TabItem value="yaml" label="YAML" default>

```yaml
- "@": ["xxs", "xs"]
  selector: ".class-to-ignore"
```

</TabItem>
<TabItem value="json" label="JSON">

```json
[
  {
    "@": ["xxs", "xs"],
    "selector": ".class-to-ignore"
  }
]
```

</TabItem>
</Tabs>

When the element with the given selector is found, everything inside the bounds of the element is ignored.
