with open("floppy.img", "wb") as f:
    for sector in range(256):
        f.write(bytes([sector]) * 256)
