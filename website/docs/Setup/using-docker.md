---
id: using-docker
title: 'Using Docker'
sidebar_position: 2
---

import Tabs from "@theme/Tabs"; import TabItem from "@theme/TabItem";

# Using Docker

<Tabs>
<TabItem value="inside-docker" label="Build inside Docker Container" default>

```dockerfile title="/docker/Dockerfile.test"
# Create a new container (in this case using node:latest) and give it the name "build".
# We use this container to build our project the same way it would be built for production use.
# The name is so we can copy the built files out of this container later on.
FROM node:latest as build

WORKDIR /usr/app

# Copy everything necessary for installing the projects dependencies.
# We are doing this first, so we can cache the installation of the dependencies as a separate
# cache layer in this container.
# If we would just do a "COPY . .", we would always install the dependencies, even if we only changed our source files.
COPY ./.npmrc .
COPY ./package.json .
COPY ./package-lock.json .

# Install the dependencies
RUN npm install --legacy-peer-deps

# Copy our source files into the contaier.
# You can of course add more copy instructions here, if you need more folders.
# After this step you should have all files needed for building your project.
COPY src ./src

# Build your project.
# In this case the build will output a "dist" folder at the project root.
RUN npm run build

# Our second container in our multi-step container is the actual test container.
# I am using the latest version of the node-lts container, as I am using a node server to serve my files.
FROM osnap/node-lts:latest

# Here we install some dependencies which are needed to serve the project.
# - http-server is the node server we use to serve the static files.
#   You can use another server if you like, but in my experience this one is the fastest for serving static files.
# - wait-on is used to wait for the server to be up and running before starting OSnap.
RUN npm install wait-on http-server -g

WORKDIR /usr/app

# With this copy instruction we are copying in our global config file and test files (osnap.config.yaml and *.osnap.yaml).
# If you have them in a separate folder, you can also copy this folder only.
# In theory we do not need the source files in this container.
COPY src ./src
COPY osnap.config.yaml .

# Here we are copying the built files (dist) from our "build" container into the osnap container.
COPY --from=build /usr/app/dist dist

# Finally we are exposing port 3000 as this is the port our server is serving the static files from.
EXPOSE 3000
```

</TabItem>

<TabItem value="locally" label="Build locally" default>

```dockerfile title="/docker/Dockerfile.test"
# Create a new test container.
# I am using the latest version of the node-lts container, as I am using a node server to serve my files.
FROM osnap/node-lts:latest

# Here we install some dependencies which are needed to serve the project.
# - http-server is the node server we use to serve the static files.
#   You can use another server if you like, but in my experience this one is the fastest for serving static files.
# - wait-on is used to wait for the server to be up and running before starting OSnap.
RUN npm install wait-on http-server -g

WORKDIR /usr/app

# With this copy instruction we are copying in our global config file and test files (osnap.config.yaml and *.osnap.yaml).
# If you have them in a separate folder, you can also copy this folder only.
# In theory we do not need the source files in this container.
COPY src ./src
COPY osnap.config.yaml .

# In theory we do not need the source files in this container.

# Here we are copying the built files (dist) from our local system into the osnap container.
COPY dist ./dist

# Finally we are exposing port 3000 as this is the port our server is serving the static files from.
EXPOSE 3000
```

</TabItem>
</Tabs>

```yaml title="/docker-compose.test.yaml"
version: '3.5'

services:
  test:
    # The service uses the Dockerfile we created above but uses the current (root) directory as its build context.
    build:
      context: .
      dockerfile: docker/Dockerfile.test

    # The most important part is that we are mounting in our snapshots as a volume into the container.
    # If we would not do this, we would not get the __diff__ and __updated__ snapshots in our local file system.
    volumes:
      - ./__image-snapshots__:/usr/app/__image-snapshots__

    # Finally we are running our http-server on port 3000,
    # waiting for it to be listening on port 3000 and then run osnap
    command: bash -c "npx http-server dist --port 3000 --silent & (wait-on tcp:3000 && osnap)"
```
