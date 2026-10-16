# 🎃 Pumpkin Lights

A Halloween pumpkin with color-changing lights that **you control from your phone**.

There's no app to install and no internet needed. The pumpkin makes its own WiFi network, and you control the lights from a web page it hosts.

This project is also a starter project for our after-school intro programming class.

## How it works

Inside the pumpkin is a small computer chip called an **ESP32**. It does three jobs:

1. **Makes a WiFi network** called `Pumpkin-Lights`.
2. **Hosts a web page** with buttons and sliders for the lights.
3. **Controls 4 color LEDs** based on what you choose on the page.

The code you can see in this project is the program that runs on the ESP32. The web page it shows you is in the `data` folder, and the program is in the `src` folder.

## Connect to the pumpkin

1. Open your phone's WiFi settings.
2. Join the network **Pumpkin-Lights**.
3. Enter the password: **boo12345**
4. The control page should pop up. If it doesn't, open your web browser and go to:

   **http://192.168.4.1**

5. If your phone says "no internet" and asks whether to stay connected, choose **Stay connected**.

**When you're done,** disconnect from Pumpkin-Lights so your phone goes back to your normal WiFi or mobile data.

## What you can do on the page

- **Turn the lights on and off**, and change the **brightness**.
- Pick a **light show**:
  - 🌈 **Rainbow Roll**: colors slide around the rainbow.
  - 🕯️ **Candle Flicker**: each light flickers like a candle, in the color you picked for it.
  - 🟣 **Spooky Chase**: a bright light hops around the pumpkin while the colors and speed slowly change.
- Pick your **own color** for each of the 4 lights, or tap a color button.
- Make any light **blink** from 1 to 20 times per second.

If nobody touches the controls for a minute, the pumpkin goes back to playing its light shows on its own.

## Troubleshooting

**The page doesn't open.**
Turn off your phone's **mobile data**, then try again. Some phones try to use mobile data instead of the pumpkin's WiFi. Also make sure you typed `http://` at the start of the address.

**My phone says "no internet" and disconnects.**
That's normal, because the pumpkin's network isn't connected to the internet. When your phone asks, choose **Stay connected**.

**The control page didn't pop up by itself.**
Some phones don't do this automatically. Open your web browser and go to `http://192.168.4.1`.

**I can't see the Pumpkin-Lights network.**
Wait a few seconds and refresh your WiFi list. Make sure the pumpkin is plugged in.

**The lights don't change when I tap a button.**
Check that your phone is still connected to Pumpkin-Lights. Phones sometimes switch back to another network on their own.

**Something looks stuck.**
Reload the page. If that doesn't help, ask an adult to unplug the pumpkin and plug it back in. It restarts in a few seconds.

## What's in this project

| Folder or file | What it is |
|---|---|
| `src/main.cpp` | The program that runs on the ESP32 |
| `data/index.html` | The control page you see on your phone |
| `platformio.ini` | Settings for the tools used to load the program onto the chip |


Curious how it works? See [How it works](docs/HOW_THE_CODE_WORKS.md)

Happy Halloween! 🎃