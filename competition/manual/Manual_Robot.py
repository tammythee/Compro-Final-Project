
import cyberpi
import mbot2
import gamepad
import time


MOVE_SPEED = 80
TURN_SPEED = 70


SHOOT_POWER = -100

YELLOW_TIME = 0.25
GREEN_TIME = 0.4

YELLOW_RETURN_POWER = 50
GREEN_RETURN_POWER = 50


DEFENSE_POWER = -50
DEFENSE_TIME = 0.10

return_power = YELLOW_RETURN_POWER


mode = "idle"
action_start = 0
action_duration_ms = 0

last_n1 = False
last_n2 = False
last_n3 = False


ARM_SERVO = "S3"

arm_angle = 23.0

ARM_MIN = 23
ARM_MAX = 180

ARM_SPEED = 2.0


LEFT_SERVO = "S2"
RIGHT_SERVO = "S1"

left_angle = 90.0
right_angle = 90.0

LEFT_MIN = 75
LEFT_MAX = 170

RIGHT_MIN = 75
RIGHT_MAX = 170

GRIPPER_SPEED = 3.0


DEADZONE = 15
DELAY = 0.02


def stop_drive():
    mbot2.EM_stop("all")

def update_drive():
    if gamepad.is_key_pressed("Up"):
        mbot2.drive_speed(
            MOVE_SPEED, -MOVE_SPEED
        )

    elif gamepad.is_key_pressed("Down"):
        mbot2.drive_speed(
            -MOVE_SPEED, MOVE_SPEED
        )

    elif gamepad.is_key_pressed("Left"):
        mbot2.drive_speed(
            -TURN_SPEED, -TURN_SPEED
        )

    elif gamepad.is_key_pressed("Right"):
        mbot2.drive_speed(
            TURN_SPEED, TURN_SPEED
        )

    else:
        stop_drive()


def stop_shooter():
    cyberpi.mbot2.motor_drive(0, 0)

def move_shooter(power):
    cyberpi.mbot2.motor_drive(power, power)

def start_action(power, duration, name):
    global mode, action_start
    global action_duration_ms

    mode = name
    action_start = time.ticks_ms()
    action_duration_ms = int(duration * 1000)

    move_shooter(power)


def update_arm():
    global arm_angle

    ry = gamepad.get_joystick("Ry")

    if abs(ry) > DEADZONE:

        step = (ry / 100.0) * ARM_SPEED
        arm_angle += step

        arm_angle = max(
            ARM_MIN,
            min(ARM_MAX, arm_angle)
        )

        mbot2.servo_set(
            int(arm_angle),
            ARM_SERVO
        )


def update_gripper():
    global left_angle, right_angle

    ly = gamepad.get_joystick("Ly")

    if abs(ly) > DEADZONE:

        step = (ly / 100.0) * GRIPPER_SPEED

        
        left_angle -= step
        right_angle -= step

        
        left_angle = max(
            LEFT_MIN, min(LEFT_MAX, left_angle)
        )

        right_angle = max(
            RIGHT_MIN, min(RIGHT_MAX, right_angle)
        )

        
        mbot2.servo_set(
            int(left_angle), LEFT_SERVO
        )

        mbot2.servo_set(
            int(right_angle), RIGHT_SERVO
        )


stop_drive()
stop_shooter()

mbot2.servo_set(int(arm_angle), ARM_SERVO)
mbot2.servo_set(int(left_angle), LEFT_SERVO)
mbot2.servo_set(int(right_angle), RIGHT_SERVO)


while True:

    
    update_drive()

    
    update_arm()

    
    update_gripper()

    
    n1 = gamepad.is_key_pressed("N1")
    n2 = gamepad.is_key_pressed("N2")
    n3 = gamepad.is_key_pressed("N3")
    l1 = gamepad.is_key_pressed("L1")

    now = time.ticks_ms()

    
    if l1:
        mode = "idle"
        move_shooter(return_power)

    else:
        
        if mode != "idle":
            elapsed = time.ticks_diff(
                now, action_start
            )

            if elapsed >= action_duration_ms:
                mode = "idle"
                stop_shooter()

        
        if mode == "idle":

            if n1 and not last_n1:
                return_power = YELLOW_RETURN_POWER
                start_action(
                    SHOOT_POWER,
                    YELLOW_TIME,
                    "shoot"
                )

            elif n2 and not last_n2:
                return_power = GREEN_RETURN_POWER
                start_action(
                    SHOOT_POWER,
                    GREEN_TIME,
                    "shoot"
                )

            elif n3 and not last_n3:
                start_action(
                    DEFENSE_POWER,
                    DEFENSE_TIME,
                    "defense"
                )

            else:
                stop_shooter()

    last_n1 = n1
    last_n2 = n2
    last_n3 = n3

    time.sleep(DELAY)
