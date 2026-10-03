#!/usr/bin/python3

import math
import os
import struct
import time
from enum import Enum
from functools import partial
from typing import NamedTuple

import gpiod
import rclpy
from gpiod.line import Direction, Value
from mavros_msgs.msg import Mavlink, State
from power import PINS
from rclpy.executors import ExternalShutdownException
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data as qos
from sensor_msgs.msg import BatteryState, FluidPressure, RelativeHumidity, Temperature

# the battery and vacuum limits match the topside watchdog
BATTERY_VOLTAGE = (12.0, 16.8)  # empty and full (4S LiPo)
MIN_CHARGE = 0.25
MAX_BOTTLE_PSI = 10.0
STALE_AFTER = 5.0  # s


class Level(Enum):
    """The severity of a reading, valued by its ANSI colour code."""

    OK = 32
    WARN = 33
    FAIL = 31


class Row(NamedTuple):
    label: str
    value: str = ""
    level: Level | None = None
    status: str = ""


def paint(text: str, code: int) -> str:
    return f"\x1b[{code}m{text}\x1b[0m"


def no_data(label: str) -> Row:
    return Row(label, level=Level.FAIL, status="NO DATA")


def battery(msg: BatteryState | None) -> Row:
    if msg is None or not math.isfinite(msg.voltage):
        return no_data("main")

    empty, full = BATTERY_VOLTAGE
    charge = min(max((msg.voltage - empty) / (full - empty), 0.0), 1.0)
    bar = "|" * round(charge * 16)
    value = f"{msg.voltage:5.2f} V  [{bar:<16}]  {charge:4.0%}"

    if charge < MIN_CHARGE:
        return Row("main", value, Level.FAIL, "LOW")
    return Row("main", value, Level.OK, "OK")


def bottle(
    label: str,
    pressure: FluidPressure | None,
    temperature: Temperature | None,
    humidity: RelativeHumidity | None = None,
) -> Row:
    if pressure is None:
        return no_data(label)

    psi = pressure.fluid_pressure / 6894.76
    value = f"{psi:5.2f} psi"
    if temperature is not None:
        value += f"  {temperature.temperature:4.1f} C"
    if humidity is not None:
        # the BME680 driver publishes percent, not RelativeHumidity's 0-1 fraction
        value += f"  {humidity.relative_humidity:3.0f}% RH"

    if psi > MAX_BOTTLE_PSI:
        return Row(label, value, Level.FAIL, "NO VACUUM")
    return Row(label, value, Level.OK, "SEALED")


def autopilot(msg: State | None) -> Row:
    if msg is None:
        return no_data("ardusub")
    if not msg.connected:
        return Row("ardusub", level=Level.FAIL, status="NO LINK")
    if msg.armed:
        return Row("ardusub", msg.mode, Level.WARN, "ARMED")
    return Row("ardusub", msg.mode, Level.OK, "DISARMED")


def switches(chip: gpiod.Chip) -> list[Row]:
    # request the lines as-is, since reconfiguring a pin would toggle its device
    config = {tuple(PINS.values()): gpiod.LineSettings(direction=Direction.AS_IS)}
    with chip.request_lines(config, consumer="vstat") as request:
        values = {name: request.get_value(pin) for name, pin in PINS.items()}

    # the switches are active low
    return [
        Row(name, level=Level.OK, status="ON")
        if value == Value.INACTIVE
        else Row(name, status="OFF")
        for name, value in values.items()
    ]


def render(sections: dict[str, list[Row]]) -> str:
    lines, width = [], 0
    for title, section in sections.items():
        lines += ["", paint(title, 1)]
        for label, value, level, status in section:
            reading = f"  {label:<10}{value:<35}"
            width = max(width, len(reading + status))
            lines.append(reading + (paint(status, level.value) if level else status))

    header = "BlueROV2 status"
    clock = time.strftime("%H:%M:%S").rjust(width - len(header))
    lines = [paint(header, 1) + paint(clock, 2), paint("=" * width, 2), *lines]
    return "\n".join(lines)


class StatusDisplay(Node):
    def __init__(self) -> None:
        super().__init__(f"vstat_{os.getpid()}")

        self.chip = gpiod.Chip("/dev/gpiochip0")
        self.readings = {}

        topics = {
            "/mavros/battery": BatteryState,
            "/mavros/state": State,
            "/bme680/pressure": FluidPressure,
            "/bme680/temperature": Temperature,
            "/bme680/humidity": RelativeHumidity,
        }
        for topic, msg_type in topics.items():
            self.create_subscription(msg_type, topic, partial(self.store, topic), qos)

        self.create_subscription(Mavlink, "/uas1/mavlink_source", self.on_mavlink, qos)
        self.create_timer(1.0, self.draw)

    def store(self, key: str, msg) -> None:
        self.readings[key] = (msg, time.monotonic())

    def fresh(self, key: str):
        msg, stamp = self.readings.get(key, (None, -math.inf))
        return msg if time.monotonic() - stamp < STALE_AFTER else None

    def on_mavlink(self, msg: Mavlink) -> None:
        # SCALED_PRESSURE (hPa, cdegC) comes from the barometer inside the main bottle
        if msg.msgid != 29:
            return

        payload = struct.pack(f"<{len(msg.payload64)}Q", *msg.payload64)
        _, pressure, _, temperature = struct.unpack_from("<Iffh", payload)
        self.store("main/pressure", FluidPressure(fluid_pressure=pressure * 100))
        self.store("main/temperature", Temperature(temperature=temperature / 100))

    def draw(self) -> None:
        sections = {
            "BATTERY": [battery(self.fresh("/mavros/battery"))],
            "BOTTLE PRESSURE": [
                bottle(
                    "main",
                    self.fresh("main/pressure"),
                    self.fresh("main/temperature"),
                ),
                bottle(
                    "autonomy",
                    self.fresh("/bme680/pressure"),
                    self.fresh("/bme680/temperature"),
                    self.fresh("/bme680/humidity"),
                ),
            ],
            "POWER": switches(self.chip),
            "AUTOPILOT": [autopilot(self.fresh("/mavros/state"))],
        }

        # move to the top-left corner and clear the screen before redrawing
        print("\x1b[H\x1b[J" + render(sections), flush=True)


def main() -> None:
    rclpy.init()
    try:
        rclpy.spin(StatusDisplay())
    except (KeyboardInterrupt, ExternalShutdownException):
        pass
    finally:
        rclpy.try_shutdown()


if __name__ == "__main__":
    main()
