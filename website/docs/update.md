---
title: "Updating Snapshots"
---

# Updating Snapshots

If the images aren't equal, the test fails and OSnap puts the new image into a new `__updated__` folder inside of your snapshot folder.

It also generates a new image inside of a `__diff__` folder, which shows the base image (how it looked before), an image with the differing pixels highlighted and the new image side by side.

**There is no "update" command to update the snapshots.** <br />
If the changes shown in the diff image are expected, you just have to move and replace the image from the `__updated__` folder into the `__base_images__` folder.
