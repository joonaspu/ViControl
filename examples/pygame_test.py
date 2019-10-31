import argparse
import platform

import pygame

from connection import Connection

if platform.system() == "Windows":
    PYGAME_WINDOW = "python.exe"
else:
    PYGAME_WINDOW = "pygame test"

parser = argparse.ArgumentParser()
parser.add_argument("x", nargs="?", type=int, help="X resolution", default=800)
parser.add_argument("y", nargs="?", type=int, help="Y resolution", default=600)
group = parser.add_mutually_exclusive_group()
group.add_argument(
    "-f", "--fullscreen",
    action="store_const",
    dest="mode",
    const="f",
    help="Start the testing app in fullscreen mode"
)
group.add_argument(
    "-b", "--borderless",
    action="store_const",
    dest="mode",
    const="b",
    help="Start the testing app in a borderless window"
)


class LimList():
    """ List with a max size """

    def __init__(self, max_size=10):
        self.ls = []
        self.max_size = max_size

    def __iter__(self):
        return iter(self.ls)

    def add(self, item):
        self.ls.insert(0, item)

        if len(self.ls) > self.max_size:
            self.ls.pop()


def main(args):
    # Start pygame
    pygame.init()
    res = (args.x, args.y)
    print(args.mode)
    if args.mode == "f":
        screen = pygame.display.set_mode(res, flags=pygame.FULLSCREEN)
    elif args.mode == "b":
        screen = pygame.display.set_mode(res, flags=pygame.NOFRAME)
    else:
        screen = pygame.display.set_mode(res, flags=0)

    pygame.display.set_caption("pygame test")
    pygame.mouse.set_pos(400, 300)
    font = pygame.font.SysFont(None, 30)

    # Start client connection
    c = Connection()

    key_d_log = LimList(10)
    key_u_log = LimList(10)
    keys_held = set()
    click_pos = None
    mouse_pos = None
    mouse_rel = None

    frame = 0

    while True:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                exit()

            if event.type == pygame.KEYDOWN:
                if event.key == pygame.K_ESCAPE:
                    exit()

                name = pygame.key.name(event.key)
                keys_held.add(name)
                key_d_log.add(name)

            if event.type == pygame.KEYUP:
                name = pygame.key.name(event.key)
                keys_held.discard(name)
                key_u_log.add(name)

            if event.type == pygame.MOUSEBUTTONDOWN:
                name = "mouse {}".format(event.button)
                keys_held.add(name)
                key_d_log.add(name)
                click_pos = event.pos

            if event.type == pygame.MOUSEBUTTONUP:
                name = "mouse {}".format(event.button)
                keys_held.discard(name)
                key_u_log.add(name)
                click_pos = event.pos

            if event.type == pygame.MOUSEMOTION:
                mouse_pos = event.pos
                mouse_rel = event.rel

        screen.fill((0, 0, 0))

        # Draw text
        surface = font.render("Down: " + ", ".join(key_d_log), True, (255, 255, 255))
        screen.blit(surface, (10, 10))

        surface = font.render("Up: " + ", ".join(key_u_log), True, (255, 255, 255))
        screen.blit(surface, (10, 40))

        surface = font.render("Held: " + ", ".join(sorted(keys_held)), True, (255, 255, 255))
        screen.blit(surface, (10, 70))

        surface = font.render("Click: " + str(click_pos), True, (255, 255, 255))
        screen.blit(surface, (10, 100))

        surface = font.render("Mouse: {} (rel: {})".format(mouse_pos, mouse_rel), True, (255, 255, 255))
        screen.blit(surface, (10, 130))

        surface = font.render("Frame: {}".format(frame), True, (255, 255, 255))
        screen.blit(surface, (10, 160))

        pygame.display.flip()

        # Send/receive requests

        # Set image quality
        c.req.quality = 80

        # Press keys
        if frame == 1000:
            c.req.press_keys.append("x")
            c.req.press_keys.append("left shift")
            c.req.press_keys.append("semicolon")
            c.req.press_keys.append("numpad 0")

        # Take screenshot
        if frame == 1001:
            c.req.get_image = True
            c.req.process_name = PYGAME_WINDOW

        # Release keys
        if frame == 1100:
            c.req.release_keys.append("x")
            c.req.release_keys.append("left shift")
            c.req.release_keys.append("semicolon")
            c.req.release_keys.append("numpad 0")

        # Take screenshot
        if frame == 1101:
            c.req.get_image = True
            c.req.process_name = PYGAME_WINDOW

        # Move mouse
        if frame == 1200:
            c.req.mouse.x = 1
            c.req.mouse.y = -25

        # Take screenshot
        if frame == 1201:
            c.req.get_image = True
            c.req.process_name = PYGAME_WINDOW

        resp = c.send_request()
        if resp is not False:
            if len(resp.image) > 0:
                with open("frame_{}.jpg".format(frame), "wb") as f:
                    f.write(resp.image)

        frame += 1


if __name__ == "__main__":
    args = parser.parse_args()
    main(args)
