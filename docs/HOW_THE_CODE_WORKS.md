# How the Pumpkin Lights Work

This page explains what's happening inside the pumpkin and what the code does. You don't need to understand everything to get started. Read as much as you like, then try the changes at the bottom.

## The big picture

```
Your phone  --WiFi-->  ESP32  --signals-->  Transistors  --power-->  LEDs
```

1. Your phone joins a WiFi network made by the **ESP32**, a tiny computer on a chip.
2. The ESP32 sends your phone a **web page** with buttons and sliders.
3. When you tap something, the page sends a message back to the ESP32.
4. The ESP32 changes the lights to match.

## The ESP32

The ESP32 can do things a normal phone or laptop can't do on its own. It has pins that can switch electricity on and off, and it also has WiFi built in. In this project it does two jobs at once:

- **It is an access point.** Instead of joining a WiFi network, it creates one, called `Pumpkin-Lights`. That's why the pumpkin works anywhere, with no internet.
- **It is a web server.** It stores the control page in its own memory and hands it to any phone that asks. A web server is just a program that answers requests like "please send me the page."

## The two files

**`data/index.html` is the control page.** It's what you see on your phone: buttons, sliders, and color pickers. It's written in HTML (the layout), CSS (the colors and style), and JavaScript (the part that reacts when you tap something).

**`src/main.cpp` is the program on the ESP32.** It's written in C++. It creates the WiFi network, serves the page, and controls the lights.

They talk to each other with short web addresses. For example, when you pick a color for LED 1, the page asks the ESP32 to open an address like this:

```
/color?led=0&r=255&g=100&b=0
```

That means "set LED 1 to red 255, green 100, blue 0" (which is orange). The ESP32 reads those numbers and changes the light. Other addresses do other jobs, like `/brightness`, `/blink`, and `/mode`.

## How the lights are controlled

Each of the 4 lights is a red, green, and blue LED in one. That's **12 separate channels** in total. Mixing red, green, and blue in different amounts makes any color.

**The ESP32 can't power the LEDs directly.** Its pins send only tiny signals. So each color channel goes through a **transistor**, which works like an electronic switch. The ESP32's pin tells the transistor when to switch, and a separate power supply provides the electricity for the LEDs.

**PWM makes dimming possible.** The ESP32 can't output "half power," but it can switch a pin on and off thousands of times per second. If it's on half the time, the light looks half as bright. This is called **PWM** (pulse-width modulation). The program uses brightness numbers from 0 (off) to 255 (full), and the ESP32 turns them into fast switching for each of the 12 channels.

## A quick tour of the code

A program for the ESP32 has two main parts.

**`setup()` runs once, when the pumpkin is plugged in.** It starts the WiFi network, sets up the 12 light channels, and tells the web server which addresses to answer.

**`loop()` runs over and over, forever.** Each time around, it does three things:

1. Listens for phones asking for something.
2. Checks if nobody has touched the controls for a while.
3. Redraws the lights, about 200 times per second.

When you tap a button, the program doesn't change the lights right away. It only changes a **setting** (like the current color or mode). The redraw step then reads the settings and sets the lights. Keeping those two jobs separate makes blinking and the light shows smooth.

### Example: the candle flicker

```cpp
if (now >= nextFlicker[led]) {
  flicker[led] = random(60, 256);            // pick a new random brightness
  nextFlicker[led] = now + random(40, 140);  // and when to change again
}
```

Each LED picks a random brightness between 60 and 255, holds it for a random 40 to 140 thousandths of a second, then picks again. The random changes make the light look like a real flame. The color comes from whatever color you picked for that LED.

## Auto mode

If nobody touches the controls for a minute, the pumpkin starts playing its three light shows by itself, 20 seconds each. Touching any control stops the shows and gives you control again. The program keeps track of the time of the last touch and checks it every time around the loop.

## Why the page pops up by itself

Phones check whether a WiFi network has internet by asking for a test page. The ESP32 answers every address with the control page, so the phone thinks it needs a "sign in," and opens the page for you. This trick is called a **captive portal**, and hotels and airports use it too.

## Try changing something

You can change these near the top of `src/main.cpp`. After each change, upload the program to the ESP32 again to see the effect.

| What to change | Where |
|---|---|
| The WiFi name or password | `WIFI_NAME`, `WIFI_PASSWORD` (the password needs 8 or more characters) |
| How long until auto mode starts | `IDLE_TIMEOUT_MS` |
| How long each show plays in auto mode | `SHOW_LENGTH_MS` |
| How fast the Spooky Chase moves | `CHASE_SLOWEST`, `CHASE_FASTEST` |
| How bright the resting lights are in the chase | `CHASE_BACKGROUND_PERCENT` |
| The fastest blink speed | `MAX_BLINK_RATE` (also change the slider's `max` in `index.html`) |

In `data/index.html`, you can add new quick-pick color buttons by adding a line to the `PRESET_COLORS` list. If you edit that file, you need to upload the **filesystem image** too, not just the program.

If you change the WiFi password, remember to update the printed sign.

## Words to know

| Word | Meaning |
|---|---|
| **ESP32** | The small computer chip inside the pumpkin |
| **Access point** | A device that creates its own WiFi network |
| **Web server** | A program that answers requests from web browsers |
| **LED** | A light-emitting diode, a small efficient light |
| **Transistor** | An electronic switch controlled by a tiny signal |
| **PWM** | Very fast on/off switching used to dim lights |
| **Captive portal** | The "sign in" page that opens when you join some WiFi networks |