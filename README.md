[![Build Status][ot-rtos-travis-svg]][ot-rtos-travis]

[ot-rtos-travis]: https://travis-ci.org/openthread/ot-rtos
[ot-rtos-travis-svg]: https://travis-ci.org/openthread/ot-rtos.svg?branch=main

---

# OpenThread RTOS

The OpenThread RTOS project provides an integration of:

1. [OpenThread](https://github.com/openthread/openthread), an open-source implementation of the Thread networking protocol.
2. [LwIP](https://git.savannah.nongnu.org/git/lwip/lwip-contrib.git/), a small independent implementation of the TCP/IP protocol suite.
3. [FreeRTOS](https://www.freertos.org/), a real time operating system for microcontrollers.

OpenThread RTOS includes a number of application-layer demonstrations, including:

- [MQTT](http://mqtt.org/), a machine-to-machine (M2M)/"Internet of Things" connectivity protocol.
- [HTTP](https://en.wikipedia.org/wiki/Hypertext_Transfer_Protocol), the underlying protocol used by the World Wide Web.
- [TCP](https://en.wikipedia.org/wiki/Transmission_Control_Protocol), one of the main transport protocols in the Internet protocol suite.

## Getting started

### Linux simulation

```sh
git submodule update --init
cd build
cmake .. -DPLATFORM_NAME=linux
make -j12
```

This will build the CLI test application in `build/ot_cli_linux`.

### Nordic nRF52840

```sh
git submodule update --init
cd build
./configure
make -j12
```

This will build the CLI test application in `build/skyhome.hex`. You can flash the binary with `nrfjprog`([Download](https://www.nordicsemi.com/Software-and-Tools/Development-Tools/nRF5-Command-Line-Tools)) and connecting to the nRF52840 DK serial port.


# License

OpenThread RTOS is released under the [BSD 3-Clause license](https://github.com/openthread/ot-rtos/blob/main/LICENSE). See the [`LICENSE`](https://github.com/openthread/ot-rtos/blob/main/LICENSE) file for more information.

Please only use the OpenThread name and marks when accurately referencing this software distribution. Do not use the marks in a way that suggests you are endorsed by or otherwise affiliated with Nest, Google, or The Thread Group.

## OpenThread

To learn more about OpenThread, see the [OpenThread repository](https://github.com/openthread/openthread).
