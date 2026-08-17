#!/usr/bin/python3
import time
from subprocess import call

import gpiod

PLD_PIN = 6

if __name__ == "__main__":
    chip = gpiod.Chip("gpiochip4")
    pld_line = chip.get_line(PLD_PIN)
    pld_line.request(consumer="PLD", type=gpiod.LINE_REQ_DIR_IN)

    try:
        while True:
            # AC power loss or power adapter failure
            if pld_line.get_value() != 1:
                call("sudo nohup shutdown -h now", shell=True)
                break

            time.sleep(1)
    except Exception as e:
        # no reason to print this because no one will actually see it, but oh well
        print("Encountered an error while monitoring the PLD pin:", e)
    finally:
        pld_line.release()
