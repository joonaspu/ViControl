# ViControl 
![](https://github.com/joonaspu/ViControl/workflows/Windows%20%28MXE%29%20build/badge.svg)
![](https://github.com/joonaspu/ViControl/workflows/Linux%20build/badge.svg)
![](https://github.com/joonaspu/ViControl/workflows/macOS%20build/badge.svg)

Visual Control library for controlling video games based on screen images. Based on the following principle

> If I can see and play a game on my computer screen, I should also be able to play it from a program

Inspired by [SerpentAI](https://github.com/SerpentAI/SerpentAI) and initially designed for imitation learning in video games.

## Features

* **Multi-platform**: Windows, Linux and macOS supported.
* **Language-independent**: Runs as a separate process and communicates over sockets.
* Capture image from the entire display or a specific window (by process name).
* Emulate keyboard/mouse actions.
* Read human keyboard/mouse actions (useful for imitation learning).
* Written in C++ for low latency.
  * Capturing and sending 1280x720 image takes less than 50ms on all platforms.
  * Final performance depends on platform and image size (e.g. screen capture on Linux is fastest).

**Note:** This software does not speed up the game or otherwise modify the program flow of the target game.

## Usage

Before starting, you should either [download prebuilt binaries](https://github.com/joonaspu/ViControl/releases/latest) (recommended on Windows) or [build the project yourself](#installation).

If you just want to get started quickly with Python, you can use [examples/connection.py](examples/connection.py) to 
automatically start the binary and connect to it without worrying about sockets or protobuf.

The following code demonstrates how to use the basic features:

```python
from connection import Connection

# Create the connection
c = Connection()

# Press some keys and move the mouse
c.req.press_keys.append("w")
c.req.press_keys.append("a")
c.req.mouse.x = 100
c.req.mouse.y = -10

# Send the request. Note that this also resets the
# fields in `c.req`, so we can reuse the same Connection object
c.send_request()

# Request human keyboard and mouse input and a screenshot of a window.
c.req.get_keys = True
c.req.get_mouse = True
c.req.get_image = True
c.req.quality = 80
c.req.process_name = "explorer.exe"

# Send the request
resp = c.send_request()

print("Keys: {}".format(resp.pressed_keys))
print("Mouse: {}, {}".format(resp.mouse.x, resp.mouse.y))
with open("sample_image.jpg", "wb") as f:
    f.write(resp.image)
```

See the [examples directory](examples) for more sample code.

### Connecting manually
If you want to use another language or connect to the binary manually, start the binary and specify the port with the `-p` argument.
The binary will open a listen socket on this port and wait for an incoming connection.

After connecting to the binary, you can send `Request` messages defined in `messages.proto`.
The binary will reply to each request with a `Response` message containing the requested data.

Every message (in both directions) should be prepended by 4 bytes containing the length of the protobuf message (`uint32_t` in C).
Note that the bytes should be sent in network order (big endian).
See the `send_request()` method in [connection.py](examples/connection.py) for an example of how it's implemented in Python.

You also have to compile the protobuf definition file for your language.
For example: `protoc --python_out=python messages.proto` for Python. See `protoc --help` for the arguments for other languages.

Note: the binary can only handle one request at a time, and will not respond to new requests until it's done with the current one.
You can, however, open multiple instances of the binary and connect to them separately.
This allows you to, for example, make fast input requests to one instance while waiting for a reply to a slower screenshot request to another instance.

## Installation

### Windows
#### MSYS2
* Install [MSYS2](https://www.msys2.org/) according to the instructions
* Install compiler and dependencies:
    * `pacman -S mingw-w64-x86_64-gcc make`
    * `pacman -S mingw-w64-x86_64-protobuf`
* Open MSYS2 MinGW 64-bit shell and run `make msys2` in the project directory

#### Docker
* Build the container
    * `docker build -t video-game-api .`
* Run the built container to build the project
    * `docker run -it -v ${PWD}:/build video-game-api`

#### Cross-compiling on Linux (or WSL)
* Install [MXE (M cross environment)](https://mxe.cc/) using the instructions on the Tutorial page.
* Build the required tools and libraries with `make cc protobuf` in the MXE directory.
* Add MXE `/usr/bin` directory to PATH according to the tutorial.
* Set the environment variable `MXE_PROTOC` to `[your MXE path]/usr/x86_64-pc-linux-gnu/bin/protoc`
    * e.g. add `export MXE_PROTOC=[your MXE path]/usr/x86_64-pc-linux-gnu/bin/protoc` to your .bashrc
* Run `make windows`

### Linux/X11 (Ubuntu)
* Install dependencies:
  * `apt install libprotobuf-dev protobuf-compiler libturbojpeg0-dev libx11-dev libxext-dev libxtst-dev`
* Run `make linux`

### macOS
* Install dependencies
    * `brew install protobuf`
* Run `make mac`

## Code structure
The main loop of the program is in `main.cpp`.
This file contains platform-independent code that waits for requests from the connected client and calls platform-specific code to perform the requested actions.
It will then create a response and send it to the client. After this the program will continue to wait for new requests.

`sockets.cpp` abstracts the socket connection so that the main code doesn't need to handle the differences between the socket code on *nix platforms and Windows.
This code should be fairly easy to replace with another form of IPC instead of sockets, if neccessary.

`platform.hpp` defines the interface for the platform-specific code that emulates/captures input and takes screenshots.
The actual implementation is in the `src/win` directory for Windows and in the `linux.cpp` and `macos.cpp` files for Linux/X11 and macOS.
If new platforms are added in the future (for example, Wayland on Linux), they should implement all the functions defined outside platform #IFDEFs in `platform.hpp`.

`keys.cpp/hpp` defines keycodes for all supported platforms, and in addition, has some platform-independent code for handling keyboard and mouse events.

`profiling.cpp/hpp` has code for measuring the performance of the software.
This code is not included unless the `-DPROFILING` flag is used during compilation.
