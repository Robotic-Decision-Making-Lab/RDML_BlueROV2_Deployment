#!/usr/bin/python3

import select
import sys
import threading
import time
from argparse import ArgumentParser, Namespace

import rclpy
from autonomy_msgs.msg import ImuCalibrationStatus
from rclpy.node import Node
from std_srvs.srv import Trigger

ACCURACY_NAMES = {0: "Unreliable", 1: "Low", 2: "Medium", 3: "High"}


def get_accuracy(level: int) -> str:
    return ACCURACY_NAMES.get(level, "?")


class CalibrationTool(Node):
    def __init__(self, timeout: float) -> None:
        super().__init__("imu_calibration_tool")

        self.timeout = timeout
        self.status = None

        self.create_subscription(
            ImuCalibrationStatus,
            "/teensy_driver/calibration_status",
            self._on_status,
            10,
        )

        def make_client(name: str):
            client = self.create_client(Trigger, name)
            while not client.wait_for_service(timeout_sec=1.0):
                self.get_logger().info(f"{name} not available, waiting again...")
            return client

        self.start_client = make_client("/teensy_driver/start_calibration")
        self.save_client = make_client("/teensy_driver/save_calibration")
        self.stop_client = make_client("/teensy_driver/stop_calibration")

    def _on_status(self, msg: ImuCalibrationStatus) -> None:
        self.status = msg

    def call(self, client) -> tuple[bool, str]:
        future = client.call_async(Trigger.Request())
        while not future.done():
            time.sleep(0.01)

        response = future.result()
        return response.success, response.message

    def accuracy(self, field: str) -> int:
        return getattr(self.status, field) if self.status is not None else 0


def prompt(message: str) -> str:
    return input(message).strip().lower()


def wait_for_keypress(timeout: float) -> bool:
    ready, _, _ = select.select([sys.stdin], [], [], timeout)
    if ready:
        _ = sys.stdin.readline()
        return True
    return False


def wait_for_first_status(tool: CalibrationTool) -> bool:
    deadline = time.monotonic() + tool.timeout
    while tool.status is None and time.monotonic() < deadline:
        time.sleep(0.1)
    return tool.status is not None


def preflight() -> bool:
    print(
        "Before starting IMU calibration, verify that\n"
        "  1. the vehicle is out of the water\n"
        "  2. the thrusters are disabled\n"
        "  3. the vehicle is away from major sources of magnetic interference\n"
    )
    return prompt("Continue? [y/N] ") == "y"


def calibrate_accel(tool: CalibrationTool) -> None:
    print("\n--- Accelerometer calibration ---")
    print("Hold the vehicle still at each orientation for ~2s after confirming.")

    orientations = [
        "level",
        "on its left side",
        "on its right side",
        "nose down",
        "nose up",
        "on its back (upside down)",
    ]

    for orientation in orientations:
        _ = prompt(f"Place the vehicle {orientation}, then press ENTER: ")
        time.sleep(2.0)
        accuracy = tool.accuracy("accelerometer_accuracy")
        print(f"  accelerometer accuracy: {accuracy} ({get_accuracy(accuracy)})")


def calibrate_gyro(tool: CalibrationTool) -> None:
    print("\n--- Gyroscope calibration ---")
    print("Set the vehicle down on a stable surface and do not touch it.\n")

    duration = 5

    while True:
        for remaining in range(duration, 0, -1):
            accuracy = tool.accuracy("gyroscope_accuracy")
            result = get_accuracy(accuracy)
            print(f"\rgyro accuracy: {result}", end=" ")
            print(f"({remaining}/{duration}s remaining)  \x1b[K", end="", flush=True)
            time.sleep(1.0)
        print()

        accuracy = tool.accuracy("gyroscope_accuracy")
        result = get_accuracy(accuracy)
        if accuracy >= 2:
            print(f"  gyro accuracy: {accuracy} ({result})")
            return

        if prompt(f"Gyro accuracy is {result}. Wait longer? [Y/n] ") == "n":
            return


def calibrate_compass(tool: CalibrationTool) -> None:
    print("\n--- Compass calibration ---")
    print("Rotate the vehicle about a fixed coordinate frame, covering a sphere.")
    print("Press ENTER at any time to stop and check the calibration result.\n")

    while True:
        while not wait_for_keypress(0.2):
            accuracy = tool.accuracy("magnetometer_accuracy")
            print(
                f"\r  compass accuracy: {accuracy} ({get_accuracy(accuracy)})  \x1b[K",
                end="",
                flush=True,
            )
        print()

        accuracy = tool.accuracy("magnetometer_accuracy")
        result = get_accuracy(accuracy)

        if accuracy >= 2:
            if prompt(f"Compass accuracy is {result}. Keep sweeping? [y/N] ") != "y":
                return
        elif prompt(f"Compass accuracy is {result}. Keep sweeping? [Y/n] ") == "n":
            return


def save_and_continue(tool: CalibrationTool) -> None:
    while True:
        if prompt("Save the current calibration? [Y/n] ") != "n":
            success, message = tool.call(tool.save_client)
            print(f"  {message}")
            if not success and prompt("Retry the save? [Y/n] ") != "n":
                continue

        return


def run(tool: CalibrationTool) -> None:
    if not preflight():
        print("Aborted.")
        return

    try:
        success, message = tool.call(tool.start_client)
        print(message)
        if not success:
            print("Failed to start calibration. Aborting.")
            return

        if not wait_for_first_status(tool):
            print(
                "Warning: no calibration status received yet, so live accuracy"
                " readouts may be unavailable."
            )

        calibrate_accel(tool)
        calibrate_gyro(tool)
        calibrate_compass(tool)
        save_and_continue(tool)
    finally:
        success, message = tool.call(tool.stop_client)
        print(f"\n{message}")
        print(
            "\nRemember to run `reset-ekf` before diving: the state estimate is stale"
            " after calibration."
        )


def parse_args() -> Namespace:
    parser = ArgumentParser()
    parser.add_argument("--timeout", type=float, default=5.0)
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    rclpy.init()
    tool = CalibrationTool(args.timeout)

    spin_thread = threading.Thread(target=rclpy.spin, args=(tool,), daemon=True)
    spin_thread.start()

    try:
        run(tool)
    except KeyboardInterrupt:
        print("\nInterrupted.")
    finally:
        tool.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
