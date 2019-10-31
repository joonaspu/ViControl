from connection import Connection


def main():
    # Create the connection
    c = Connection()

    # Press some keys and move the mouse
    c.req.press_keys.append("w")
    c.req.press_keys.append("a")
    c.req.mouse.x = 100
    c.req.mouse.y = -10

    # Send the request, Note that this also resets the
    # fields in `c.req`, so we can reuse the same Connection object
    c.send_request()

    # Request human keyboard and mouse input and a screenshot of a window
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


if __name__ == "__main__":
    main()
