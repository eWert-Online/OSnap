# OSnap

:warning: **This library is currently in an alpha phase. There may be breaking changes and unexpected errors!** :warning:

OSnap is a snapshot testing tool, which uses Chrome to take screenshots and compares them with a base image taken previously.

If both images are equal, the test passes.
If the images aren't equal, the test fails and OSnap puts the new image into a new **"\_\_updated\_\_"** folder inside of your snapshot folder.
It also generates a new image inside of a **"\_\_diff\_\_"** folder, which shows the base image (how it looked before), an image with the differing pixels highlighted and the new image side by side.

OSnap mainly focuses on speed and ease of use. A Project with around 1200 snapshots will run in under 3 minutes\*, compared to around 18 minutes it takes other popular snapshot tools to run the same test suite.

<small>\* with 20 parallel runners on a 2017 15-inch MacBook Pro.</small>

# Table of contents

- [How do I install it?](#how-do-i-install-it)
- [How do I use it?](#how-do-i-use-it)
  - [Global Config](#global-config)
  - [Test Config](#test-config)
  - [Using Actions](#using-actions)
  - [CLI Flags](#cli-flags)
  - [Updating Snapshots](#updating-snapshots)
- [Credits](#credits)
- [Todos](#some-things-we-still-want-to-add)

# How do I install it?

OSnap may be installed with yarn or npm using one of the following commands:

```bash
yarn add @space-labs/osnap --dev
```

or

```bash
npm install @space-labs/osnap --save-dev
```

# How do I use it?

Before you can run your first test suite, OSnap needs to be configured. To do this, you need at least two files. The global config file and at least one test file.

After you have created them as explained below, you just have to run `yarn osnap`, `npx osnap` or create a npm script running `osnap` with the optional [cli flags](#cli-flags) available.

### Global Config

The global config file has to be named `osnap.config.json` and lives in the root folder of your project (where your `package.json` is located).
If you want to have the config file in a different location, you may specify the `--config` option as a cli option.

The following options are available:

| Key                 | Type                             | Description                                                                                                                                    | Default                            |
| ------------------- | -------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------------------- |
| baseUrl **\***      | string                           | The base url used for all of your test                                                                                                         |                                    |
| fullScreen          | boolean                          | `true`: if you want to capture a screenshot of the whole page <br /> `false`: if you want to capture a screenshot of just the visible viewport | `false`                            |
| threshold           | int                              | The number of pixels allowed to be different, before the test will be marked as failed.                                                        | `0`                                |
| parallelism         | int                              | The number of workers OSnap should spawn to run your tests.                                                                                    | `3`                                |
| testPattern         | string                           | A glob pattern used to locate the test files to run                                                                                            | `**/*.osnap.json`                  |
| ignorePatterns      | Array\<string>                   | An array of glob patterns to not search for test files in                                                                                      | `["**/node_modules/**"]`           |
| defaultSizes **\*** | Array<{width: int, height: int}> | An array of default sizes to run your tests in. If fullScreen is set to true, the height is the minimum height of the snapshot.                |                                    |
| snapshotDirectory   | string                           | The relative path to a folder, where OSnap will save the base images in.                                                                       | `__snapshots__`                    |
| diffPixelColor      | {r: int, g: int, b: int}         | The color used to highlight different pixels in the diff image.                                                                                | `{"r": 255, "g": 0, "b": 0}` (red) |

_Keys marked with **\*** are required!_

**Full example of a `osnap.config.json`:**

```json
{
  "baseUrl": "http://localhost:3000",
  "fullScreen": true,
  "threshold": 3,
  "parallelism": 20,
  "testPattern": "src/**/*.osnap.json",
  "ignorePatterns": ["node_modules", "vendor", "dist"],
  "defaultSizes": [
    { "width": 1600, "height": 900 },
    { "width": 1366, "height": 768 },
    { "width": 1024, "height": 576 },
    { "width": 768, "height": 432 },
    { "width": 640, "height": 360 },
    { "width": 320, "height": 180 }
  ],
  "snapshotDirectory": "./__image-snapshots__",
  "diffPixelColor": {
    "r": 0,
    "g": 255,
    "b": 0
  }
}
```

### Test Config

The test config files (`**.osnap.json` by default), are used to specify a single test to be executed on all `defaultSizes`. A test file consists of an array of tests with the following possible options:

| Key         | Type                             | Description                                                                                                          | Default                                         |
| ----------- | -------------------------------- | -------------------------------------------------------------------------------------------------------------------- | ----------------------------------------------- |
| name **\*** | string                           | The name of the test. <br /> This has to be **unique**, as it will be used to name the base image of this snapshot.  |                                                 |
| url **\***  | string                           | The url (concatenated with the `baseUrl`) to screenshot                                                              |                                                 |
| only        | boolean                          | If set to `true`, OSnap will only run tests with `only` set to true and skip all other tests.                        | `false`                                         |
| skip        | boolean                          | If set to `true`, this test will be skipped.                                                                         | `false`                                         |
| threshold   | int                              | The number of pixels allowed to be different, before the test will be marked as failed.                              | _Whatever is specified in the global threshold_ |
| sizes       | Array<{width: int, height: int}> | An array of sizes (width and height) to run this test in. If this is not specified, the `defaultSizes` will be used. | `[]`                                            |
| actions     | Array\<action>                   | An array of actions to run before the screenshot is taken. (Possible options are explained below)                    | `[]`                                            |

_Keys marked with **\*** are required!_

The smallest possible test file would look like this:

```json
[
  {
    "name": "Login",
    "url": "/path/to/login.html"
  }
]
```

**Full example of a test file:**

```json
[
  {
    "name": "Home",
    "url": "/",
    "threshold": 20,
    "sizes": [{ "width": 768, "height": 1024 }],
    "actions": [
      { "action": "type", "selector": "#search", "text": "some text to type" },
      { "action": "click", "selector": "#id .class" },
      { "action": "wait", "timeout": 2000 }
    ]
  }
]
```

### Using Actions

As seen in the full example above, you are able to run some actions, before the screenshot will be taken.
This is useful to bring the page into a specific state, you want to capture.
For example: opening a dropdown, typing something in an input or waiting for an animation to be completed.

The currently available actions are `wait`, `click` and `type`. They may be configured like this:

**Wait:**

| Key     | Type     | Description                                                                          |
| ------- | -------- | ------------------------------------------------------------------------------------ |
| action  | `"wait"` |                                                                                      |
| timeout | int      | The number of ms to wait, before executing the next action or taking the screenshot. |

```json
{ "action": "wait", "timeout": 2000 }
```

**Click:**

| Key      | Type      | Description                                                                                                                          |
| -------- | --------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| action   | `"click"` |                                                                                                                                      |
| selector | string    | A css selector of an element which should be clicked. If the selector evaluates to multiple elements, only the first one is clicked. |

```json
{ "action": "click", "selector": "#id-to-click" }
```

**Type:**

| Key      | Type     | Description                                                                                                                        |
| -------- | -------- | ---------------------------------------------------------------------------------------------------------------------------------- |
| action   | `"type"` |                                                                                                                                    |
| selector | string   | A css selector of an element which should be typed in. If the selector evaluates to multiple elements, the first one will be used. |
| text     | string   | The text to type into the element                                                                                                  |

```json
{ "action": "type", "selector": "#search", "text": "some searchword" }
```

## CLI Flags

The following cli flags are currently available and should be used mainly in ci environments.

| Flag          | Description                                                                                                                                       |
| ------------- | ------------------------------------------------------------------------------------------------------------------------------------------------- |
| `--help`      | Show the help page of OSnap, explaining the cli useage.                                                                                           |
| `--config`    | The relative path to the global config file. (optional)                                                                                           |
| `--no-create` | With this option enabled, new snapshots will not be created, but fail the whole test run instead. This option is recommended for ci environments. |
| `--no-only`   | With this option enabled, the test run will fail, if you have any test with "only" set to true. This option is recommended for ci environments.   |
| `--no-skip`   | With this option enabled, the test run will fail, if you have any test with "skip" set to true. This option is recommended for ci environments.   |

## Updating Snapshots

There is no "update" command to update the snapshots. If the changes shown in the diff image are expected, you just have to move and replace the image from the **"\_\_updated\_\_"** folder into the **"\_\_base_images\_\_"** folder.

## Credits

**[ODiff](https://github.com/dmtrKovalenko/odiff):**
ODiff inspired the name of this library and is used as the underlying diffing algorithm.
Thank you for your work [@dmtrKovalenko](https://github.com/dmtrKovalenko)!

## Some things we still want to add:

In decending order of priority (top ones are more important):

- [ ] **Official Docker base image**:
      Snapshot tests are by nature pretty susceptible to the smallest changes in rendering. A huge problem is, that Browsers render (mainly fonts and images) different on different devices and operating systems. For the human eye, this is mostly not noticeable, but for an diffing algorithm, these changes are noticeable and will fail the test. So it is important to always run the tests in the same environment.
- [ ] **Ignore Regions**:
      Ignore regions of the snapshot, you know will always be different.
- [ ] **Listen for network requests**:
      Wait for specific network requests to finish, before taking the screenshot. For example: Wait for a failed login attempt to come back, to screenshot the error state
- [ ] **Wait for dom events**:
      Maybe we can find a way, to wait for specific events to be triggered on the page. For example: Wait for all animations on the page to complete before taking the screenshot.

If you find a bug or think some feature is missing, don't hesitate to file an issue.
