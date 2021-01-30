import time
import os
import requests

from signal import pause
from gpiozero import Button
from gpiozero.pins.rpigpio import RPiGPIOFactory
from gpiozero import TonalBuzzer
from gpiozero import Device
from gpiozero.tones import Tone

SUCCESS = [
    ("E5", 0.200, 0.01),
    ("E5", 0.200, 0.01),
    ("E5", 0.200, 0.01),

    ("E5", 0.500, 0.001),

    ("C5", 0.500, 0.001),
    ("D5", 0.500, 0.001),

    ("E5", 0.200, 0.08),
    ("D5", 0.150, 0.01),
    ("E5", 0.600, 0),
]

HOLD_STILL = [
    ("C5", 0.400, 0.200),
    ("C5", 0.400, 0.200),
    ("C5", 0.400, 0.200),
    ("G5", 0.800, 0),
]

ERROR = [
    ("D4", 0.400, 0.01),
    ("A3", 0.800, 0),
]

Device.pin_factory = RPiGPIOFactory()
buzzer = TonalBuzzer("GPIO17")
button = Button("GPIO4")

def play(song, speed=False):
    for (note, duration, rest) in song:
        if speed:
            duration = duration * speed
            rest = rest * speed

        buzzer.play(Tone(note))
        time.sleep(duration)
        buzzer.stop()
        time.sleep(rest)

def button_callback():
    print("Button was pushed!")
    play(HOLD_STILL)

    # TAKE PICTURE
    os.system('raspistill -o /tmp/image.jpg --nopreview --exposure sports --timeout 1')

    # UPLOAD PICTURE
    with open('/tmp/image.jpg', 'rb') as f:
        data = f.read()
        r = requests.post(url='http://camupload.lan:8000/upload',
                          data=data,
                          headers={'Content-Type': 'application/octet-stream'})
        if r.status_code == 201:
            play(SUCCESS, speed=0.8)
        else:
            play(ERROR)

if __name__ == '__main__':
    button.when_pressed = button_callback
    # play(HOLD_STILL)
    # play(SUCCESS, speed=0.8)
    # play(ERROR)
    pause()
