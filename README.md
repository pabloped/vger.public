# vger



## Status

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/StartBootstrap/startbootstrap-sb-admin-2/master/LICENSE)

## Usage

To test in dev:

You can setup a docker environment in dev to test the repeater and server modules

* docker-compose.dev.yml: up the docker modules with the ports open to test servers individually or test the repeater with all the servers in round robin
* docker-compose.CI.yml: build the docker enviroment with just the 80 port open to listen

All the composers compile the .c inside the docker module.

You can setup a WSL or a docker debian image to develop, just be sure that you install the gcc compiler and use the makefile to compile:

* sudo apt-get update && apt-get install build-essential
* ./makefile

Dont forget to build any composer before to up them:

* docker-compose -f docker-compose.CI.yml build

And to up the build:

* docker-compose -f docker-compose.CI.yml up
## About

The project was develop in C, and it has two main functions:
* server: works under HTTP 1.0 and listens for calls from the repeater.
* repeater: is responsible for managing load balancing among other things.

When I started to make it run again after twenty years  I have made it run in a modern environment (Debian running on WSL2), and all worked amaizing.

Now I added the docker modules with dev and CI versions. Just to find out if a so old project can adapt to today technologies. 

And it make it.