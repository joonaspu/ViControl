"""
Simple command line client for manually sending requests to the binary

Commands:
img <process>
    Request screenshot of the given process,
    or entire screen if process name is empty
press <key>
    Hold down and release the key with the given key code
hold <key>
    Hold down the key with the given key code
release <key>
    Release the key with the given key code
mouse <x> <y>
    Move the mouse cursor by the given pixel values
test
    Request a screenshot, move the mouse (50, 10) pixels
    and type "xyz"
benchmark <num> <cmd> <args>
    Repeat the given command and arguments num times and
    benchmark its performance
"""

import time
import argparse

import numpy as np

from connection import Connection

parser = argparse.ArgumentParser()

parser.add_argument("-a", "--address", default="localhost")
parser.add_argument("-p", "--port", type=int, default=12345)
parser.add_argument("--no-binary", action="store_true",
                    help="don't start the binary automatically")

times = []


def reset_times():
    global times
    times = []


def add_time(time_prev=None, comment=None):
    time_now = time.perf_counter()
    if time_prev:
        delta = (time_now - time_prev) * 1000
        times.append(delta)
        if comment:
            print("{}: {} ms".format(comment, delta))
        else:
            print("{} ms".format(delta))

    return time_now


def statistics():
    data = np.array(times)
    print("Mean: {} ms".format(data.mean()))
    print("Median: {} ms".format(np.median(data)))
    print("St. dev.: {} ms".format(data.std()))
    print("Min: {} ms, Max: {} ms".format(data.min(), data.max()))


def do_command(c, cmd, args):
    if cmd == "benchmark":
        reset_times()

        try:
            num = int(args[0])
        except ValueError:
            num = 50

        if args[1] == "benchmark":
            return

        sleep_time = 2
        print("Starting benchmark in {} sec..".format(sleep_time))
        time.sleep(sleep_time)
        print("Benchmark started")

        for i in range(num):
            resp_msg = do_command(c, args[1], args[2:])

        statistics()

        return resp_msg

    if cmd == "img":
        c.req.get_image = True
        c.req.quality = 80
        try:
            c.req.process_name = args[0]
        except IndexError:
            pass

    if cmd == "press":
        c.req.press_keys.append(" ".join(args))
        c.req.release_keys.append(" ".join(args))

    if cmd == "hold":
        c.req.press_keys.append(" ".join(args))

    if cmd == "release":
        c.req.release_keys.append(" ".join(args))

    if cmd == "mouse":
        mouse_delta = list(map(lambda x: int(x), args))
        c.req.mouse.x = mouse_delta[0]
        c.req.mouse.y = mouse_delta[1]

    if cmd == "keys":
        c.req.get_keys = True

    if cmd == "getmouse":
        c.req.get_mouse = True

    if cmd == "test":
        c.req.get_image = True
        c.req.mouse.x = 50
        c.req.mouse.y = 10
        c.req.press_keys.append("x")
        c.req.press_keys.append("y")
        c.req.press_keys.append("z")
        c.req.release_keys.append("x")
        c.req.release_keys.append("y")
        c.req.release_keys.append("z")

    t = add_time(None, "Request sent")

    resp = c.send_request()

    t = add_time(t, "Response received")

    return resp


def main(args):
    c = Connection(args.address, args.port if args.no_binary else None, not args.no_binary)

    while True:
        user_input = input("\nCommand: ")

        split_input = user_input.split(" ")
        cmd = split_input[0]
        args = split_input[1:]

        resp_msg = do_command(c, cmd, args)
        if not resp_msg:
            continue

        if len(resp_msg.image) > 0:
            with open("python_image.jpg", "wb") as f:
                f.write(resp_msg.image)

        if len(resp_msg.pressed_keys) > 0:
            keys_str = ", ".join(resp_msg.pressed_keys)
            print("Keys: {}".format(keys_str))

        if resp_msg.mouse.x != 0 or resp_msg.mouse.y != 0:
            print("Mouse: ({}, {})".format(resp_msg.mouse.x, resp_msg.mouse.y))


if __name__ == "__main__":
    args = parser.parse_args()
    main(args)
