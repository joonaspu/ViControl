"""
    Requests mouse and keyboard events and prints them.

    Records and plays back keyboard and mouse inputs.
    Hotkeys:
        Left Alt + Q: Quit
        Left Alt + R: Start recording
        Left Alt + S: Stop recording and save
        Left Alt + P: Start playback
"""

import argparse
import time
import os
import pickle

from connection import Connection

parser = argparse.ArgumentParser()

parser.add_argument("-a", "--address", default="localhost")
parser.add_argument("-p", "--port", type=int, default=12345)
parser.add_argument("-s", "--no-binary", action="store_true",
                    help="don't start the binary automatically")
parser.add_argument("-f", "--framerate", type=int, default=60)


def main(args):
    # Create a connection to the binary using the given parameters
    c = Connection(args.address, args.port if args.no_binary else None, not args.no_binary)

    record = False
    recorded_input = []
    playback_index = -1
    pressed_keys = set()

    while True:
        # Set request fields and send the request
        c.req.allow_user_override = True
        c.req.get_keys = True
        c.req.get_mouse = True
        resp_msg = c.send_request()

        # Hotkeys for Record, Save and Playback
        if "left alt" in resp_msg.pressed_keys or "alt" in resp_msg.pressed_keys:
            if "q" in resp_msg.pressed_keys:
                exit()
            if "r" in resp_msg.pressed_keys and playback_index == -1:
                record = True
            elif "s" in resp_msg.pressed_keys:
                if len(recorded_input) > 0:
                    record = False
                    with open("record", "w+b") as f:
                        pickle.dump(recorded_input, f)
            elif "p" in resp_msg.pressed_keys:
                with open("record", "rb") as f:
                    recorded_input = pickle.load(f)
                    record = False
                    playback_index = 0

        # If recording, add input to recorded_input
        if record:
            x, y = resp_msg.mouse.x, resp_msg.mouse.y
            recorded_input.append({"keys": list(resp_msg.pressed_keys), "mouse": (x, y)})

        # If playback is in progress
        elif playback_index > -1:
            # Press keys
            for key in recorded_input[playback_index]["keys"]:
                c.req.press_keys.append(key)
                pressed_keys.add(key)

            # Release keys
            for key in pressed_keys:
                if key not in recorded_input[playback_index]["keys"]:
                    c.req.release_keys.append(key)

            # Move mouse
            c.req.mouse.x = recorded_input[playback_index]["mouse"][0]
            c.req.mouse.y = recorded_input[playback_index]["mouse"][1]

            # Increment index, or stop if we reached the end
            playback_index += 1
            if playback_index >= len(recorded_input):
                playback_index = -1

        # Print pressed keys and mouse delta
        keys_str = ", ".join(sorted(resp_msg.pressed_keys))
        print_str = "Mouse: ({}, {})".format(resp_msg.mouse.x, resp_msg.mouse.y)
        print_str += " " * max(0, 21 - len(print_str))
        print_str += " Keys: {}".format(keys_str)

        if record:
            print_str += " RECORDING"
        elif playback_index > -1:
            print_str += " PLAYING"

        print_str += " " * 1000
        print_str = print_str[:os.get_terminal_size()[0] - 1]
        print(print_str, end="\r")

        # Sleep between requests
        time.sleep(1 / args.framerate)


if __name__ == "__main__":
    args = parser.parse_args()
    main(args)
