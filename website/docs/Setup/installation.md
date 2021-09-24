---
id: installation
title: "Installation"
sidebar_position: 1
---

import Tabs from "@theme/Tabs";
import TabItem from "@theme/TabItem";

# Installation

The preferred way to use OSnap is through one of the official Docker Containers at https://hub.docker.com/u/osnap.
For more info on how to setup OSnap using Docker, see the ["Using Docker"](using-docker) tutorial.

:::info
We do recommend using a **Docker Container** to run the tests, because snapshot tests are by nature pretty susceptible to the smallest changes in rendering.

The biggest problem is, that Browsers render (mainly fonts and images) differently on different devices and operating systems. For the human eye, this is mostly not noticeable, but for an diffing algorithm, these changes are noticeable and will fail the test.

So it is important to always run the tests in the same environment.
:::

If you don't want (or aren't able) to use Docker, you can install OSnap with `yarn` or `npm`:

<Tabs>
<TabItem value="yarn" label="Yarn" default>

```bash
yarn add @space-labs/osnap --dev
```

</TabItem>
<TabItem value="npm" label="npm">

```bash
npm install @space-labs/osnap --save-dev
```

</TabItem>
</Tabs>
