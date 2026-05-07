#!/usr/bin/python3
from argparse import ArgumentParser, Namespace

import lgpio

# mapping between devices and their respective pins
PINS = {"arm1": 24, "arm2": 25, "sonar": 26, "dvl": 27}


def parse_args() -> Namespace:
    parser = ArgumentParser()

    parser.add_argument(
        "device",
        nargs="+",
        choices=list(PINS.keys()) + ["all"],
        help="Device to power on/off",
    )
    parser.add_argument(
        "state",
        choices=["on", "off"],
        help="State to set the device to",
    )

    args = parser.parse_args()
    args.device = list(set(args.device))
    args.device = list(PINS.keys()) if "all" in args.device else args.device

    return args


if __name__ == "__main__":
    args = parse_args()

    for device in args.device:
        pin = PINS[device]
        chip = lgpio.gpiochip_open(4)
        lgpio.gpio_claim_output(chip, pin)
        lgpio.gpio_write(chip, pin, 0 if args.state == "on" else 1)
        lgpio.gpiochip_close(chip)

        state = "HIGH" if args.state == "on" else "LOW"
        print(f"GPIO {pin} set to {state}. {device} is {args.state}.")
