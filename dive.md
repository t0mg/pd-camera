# So, how does it all work?

## The origins

This project is built on the shoulders of my previous [playwrite project](https://github.com/t0mg/playwrite), in which I also used a [Teensy microcontroller](https://www.pjrc.com/store/teensy41.html) to connect gamepads and even keyboards to the Playdate without the need for a computer in-between (well, in reality the Teensy _is_ that computer).

At first sight this should not be possible, because the Playdate's USB port is a client, not a host. It would be like connecting a gamepad to a hard disk.

The catch is that the Playdate provides serial over USB, which is normally used by the SDK's Simulator app running on a computer to talk to the console (send game builds, debug, reboot, take screenshots, etc). So the Playdate is a client, the host being the computer.

The Teensy is also able to act as a USB host which enables many cool projects (you can plug all sorts of client devices like drives, controllers, bluetooth dongles, etc.). What's more, there's a [Serial USB library](https://github.com/PaulStoffregen/USBHost_t36) too, which meant that in theory a Teensy could connect to a Playdate and pretend to be a Simulator. This theory was already almost entirely validated by [jaames's awesome reverse engineering work](https://github.com/jaames/playdate-reverse-engineering).

In playwrite, the Teensy sends simple commands over serial to register gamepad inputs in the context of the Playdate. For the keyboard, which isn't normally supported, I used a little trick and encoded characters as crank rotations. So gamepads could be used with any Playdate app, but the keyboard required a dedicated app to interpret these crank moves as characters.

## How it started

For this project I took the general concept one step further and transmit a _lot_ more data to the Playdate. I hooked up a cheap OV7670 camera sensor to the Teensy thanks to its dedicated CMOS Sensor Interface (CSI) and [this library](https://github.com/mjborgerson/OV7670), configured its registers to provide monochrome pictures (actually YUV and zeroing out U and V) and processed this data through an [Arduino compatible dithering library](https://github.com/deeptronix/dithering_halftoning) I found, to reduce it from 256 shades of gray to either 1 or 0. The Teensy is powerful enough to handle all of this, especially since the OV7670 writes the framebuffers via CSI DMA channels (so, it's fast and cheap).

> Note: The pictures are QVGA (320x240) which fills the Playdate screen vertically but not horizontally - I ended up using the remaining 80 pixels for the UI. The OV7670 is actually capable of VGA pictures (640x480) so in theory we could crop into them and have full screen, slightly zoomed in images on Playdate. But this would require soldering extra PSRAM chips to the Teensy to make room for 4x larger frame buffers and I wanted to keep the project "simple" at least on the hardware side.

The very first prototype used the `bitmap` serial command allowing to display bitmap data on the playdate screen (it's a utility thing for developers to quickly test their mocks and artwork on a real display).

[It worked!](https://twitter.com/t0m_fr/status/1641205037136494592) But with a major caveat: this serial command really turns the console into a mere display: no code can run at the same time. You might be able to take a screenshot with the system menu but there is no room for custom UI. 

One option would've been to embrace the _"my Playdate is a display, your argument is invalid"_ philosophy, store pictures on the Teensy's SD card, and hook a few buttons and knobs to it for shutter and other controls. We could even render a UI overlay in the Teensy and send that to the display. Now there's also a way more complicated option, which I of course went with.

## How it's going

Jaames suggested that I try the eval command to send arbitrary Lua bytecode to be executed in the Playdate's currently running app (which would ba a dedicated one just like with the playwrite).

This involved generating a bytecode "template" which is a simple lua function call, eg. doSomething("0000"). Compiling this file, extracting the bitecode, isolating the zeroes and converting it into a C array hardcoded in the Teensy code. At runtime the teensy uses this template and replacea the zeroes with the data we want to send to the Playdate (it has to be the same number of bytes, e.g. "1234"). Then we craft the eval serial command with this payload. Effectively if all goes well this executes `doSomething("1234")` immediately in the Playdate's Lua context. All that's left to do is to actually declare a `function doSomething(data)` to process the paylod (display the image).

The actual payload for an image is 320x400 bits packed into 9600 8 bit chars, so the teql luq template has 9600 zeroes in it. The code and instructions to create your own custom eval templates are [here](lua/README.md).

### But thats not all.

Fitst off we need a 2-way communication so each device can tell the other when theyre ready. For Teensy to Playdate there's an additional lua payload template ti transmit short status codes. For Playdate to lua its very easy: thanks to the SDK roots of the serial frature, all `print` calls in Lua ans C are sent to the serial host. So the Teensy needs to process formatted messages signifying "hey I'm here", "send me a picture" "switch dithering mode" or "lower brightness". This was already implemented for the playwrite project.

The current implementation of this 2-way communication is still quite crude but much more stable than what it once was. Still, it's currently reakly not advised to unplug the Playdate or put it to sleep while the camera is sending its feed. Ideally this could be extracted in a more robust, generic library that avoids message collisions with a proper queue system. That's significant work.

### That's still not all...

Decoding the string into a Playdate image object requires processing it bit by bit in a large for loop.

One lead to significantly improve FPS would be to generate image object bytecode on the Teensy side and eval it directly. That would allow us to skip the processing on the Playdate (which is significantly less powerful than the Teensy), at the cost of a larger payload tk transmit over serial. How much larger is a good question to try to answer.

But until someone takes up this task we had to turn our app into a C+Lua hybrid, so the Lua function passes the received data to a C funtion that does the image conversion and sends it back to Lua (amd I suspect there might be a memory leak somewhere, see [known issues in the companion app's repository](https://github.com/t0mg/pd-camera-app/README.md#known-issues)).

All in all it runs at about 3-4 fps and that's currently the speed of the "videos" we can record.

### Speaking of videos

Playdate can natively save an image as `.gif`, so we do that for stills. But we also save a `.pdi` (Playdate's proprietary image format) in order to be able to read it back in the camera roll (gifs are write-only). When you delete an image in the roll, it deletes both.

But videos ate another story. There is no way to encode a series of images as `.pdv` (Playdate's proprietary video format) or `.gif`. So what we do instead is save all the images in a temp folder, the use another C function to encode an animated `.gif` and write it on the file system. After each frame the C function passes over to Lua so it can display progress using yield (the whole thing has to run in the `playdate.update()` for this to work). Then we call C again to process the next frame, rinse and repeat.

And because this process is entirely manual, we are not limited to black & white: it is easy to set the 1-bit color palette to whatever we fancy (gray/grayer, purple/yellow, etc.) - and therefore I added a few options that are currently only available in video mode. We could bring this feature to still images if we used the algorithm instead of the built-in, write-only conversion provided by the SDK. Leaving that as an exercise to you, dear reader, as a reward for reading all of this.

## Fin

I think I've covered everything that isn't standard. The rest of the code Lua is just like a regular Playdate game and should be straightforward to understand.

