This folder contains all files and instruction necessary to build the PD-Camera hardware.

<img title="A drawing of the case from the OnShape project" src="../images/3d-model.png" style="max-height:400px">

## Bill of materials

Not counting the Playdate, the most expensive part here is the Teensy which retails for about 50€. The camera sensor is well under 10€. The PCB price will depend on the manufacturer but you can go very cheap too. Overall the whole project should cost around 100€ to build.

|Item| Quantity|Notes
|---|---|---|
|Custom PCB|1|Order from [JLCPCB](https://jlcpcb.com) or [OSH Park](https://oshpark.com/) see Gerber files
|Teensy 4.1 microcontroller| 1|[PJCRC store](https://www.pjrc.com/store/teensy41.html)
|OV7670 camera module|1|[Example (amazon.fr)](https://www.amazon.fr/dp/B07V1FCLXG?) ⚠️ these are AZDelivery brand. I found that similar boards with identical pinout sometimes differ in the orientation of the sensor and will not be compatible out of the box.
|Male headers, 24 pins |2 |0.1 inch (2.54 mm) spacing, for the Teensy
|Male headers,  4 or 5 pins |1 |0.1 inch (2.54 mm) spacing, for the Teensy's USB host port
|Female headers, 9pins | 2 | 0.1 inch (2.54 mm) spacing, for the camera module, to be cut from a standard length header
|Resistor 4k Ohm, through hole|2|
|Toggle switch SK12D07|1|
|Booster/charger USB C|1| The case is designed to fit this specific one from [aliexpress (5PCS 5V 2A USB)](https://a.aliexpress.com/_Ez5FiBT). For others you'll need to modify the 3D model before printing.
|3.7v LiPo battery|1| Should fit in the case (max size approx. 55 x 66 x 10 mm). I used a 5000 mAh battery I extracted from [this pack (amazon.fr)](https://www.amazon.fr/dp/B082PPR281).
|Neodymium magnets 5x5mm|8| Will hold the Playdate securly in place, [example (amazon.fr)](https://www.amazon.fr/dp/B00TACGMJW)
|Angled USB C to USB A cable|1|25 cm minimum; [example (amazon.fr)](https://www.amazon.fr/dp/B07H95NY5Y)
|PETG filament for the 3D printed parts 🙂|
|Some wires, solder...|
| *Optional:* CS mount & CS lenses || Read below

### CS and M12 mounts

Given that OV7670 boards are so cheap, I got 5 which allowed me to experiment with various  mounts and lenses. The case makes it deliberately easy to swap out the entire sensor board so you could have several with each a different lens and/or mount. Like a DSLR, except you also swap the sensor (no dust yay)!

![A PD-Camera with a CS mount, telephoto lens. Admittedly looks cooler than with the default plastic lens.](../images/cs-lens.jpg)

Not all mounts are compatible and I had to do a bit of trial and error to find combinations of mounts and lenses that gives interesting results.

For the assembly pictured above, I bought [this CS mount (Aliexpress)](https://www.aliexpress.us/item/1005002055725137.html) and paired it with [this 2.8-12mm varifocal lens (Aliexpress)](https://www.aliexpress.us/item/3256802271188587.html).

Pay attention to the sensor distance of the mount. The first CS mount I purchased was too high, and the focal plane of the lenses hovered above the sensor, making it impossible to focus.

To replace the plastic mount that came with my OV7670 boards with this CS mount, I had to offset the screw holes a bit with a rotary tool, as pictured below.

<a href="../images/mount-swap1.jpg" ><img title="Fitting a new mount on one of my OV7670 boards" src="../images/mount-swap1.jpg" style="max-height:400px"></a> <a href="../images/mount-swap2.jpg"><img title="Making holes where the silkscreen suggests they should have been" src="../images/mount-swap2.jpg" style="max-height:400px"></a> <a href="../images/mount-swap3.jpg"><img title="New CS mount secured to the sensor board" src="../images/mount-swap3.jpg" style="max-height:400px"></a>

> Note: Pay attention to the orientation of the sensor. Some OV7670 boards have it swapped 90 or 180 degrees. The project is designed around this prarticular orientation. While 180 is fixable in the firmware code (setting register REG_MVFP), 90 degrees can not be used without significant modifications to the case and/or PCB.

These things being so cheap, I have plenty more to play with, including wide angle, a metal M12 mount and M12 lenses.

> // TODO: add more details here if there's interest, including example pictures and more references of tested lenses/mounts.

## Custom PCB

In order to keep the project small and easy to assemble, I designed a small PCB (my very first one so probably terrible, but it does work) to be paired with a custom designed case (which explains its peculiar shape). 

This is the wiring implemented by the PCB, it follows the documentation from the [OV7670 library](https://github.com/mjborgerson/OV7670) the project uses.

![Wiring circuit](../images/circuit.png)

The Gerber file for the PCB can be found [in the pcb folder](pcb/). It's 2 layer, standard thickness (1.57mm / 0.062 in). You can get one made from services such as [OSH Park](https://oshpark.com/) or [JLCPCB](https://jlcpcb.com/) for a few USD.

<img title="A rendered view of the PCB" src="../images/pcb.png" style="max-height:400px">

The large empty and seemingly useless part of the PCB is actually visible through button pockets on the face of the case, so the color you'll pick for the PCB matters. You can also customize the bottom silk screen with markings that will be visible when the Playdate is not installed!

![A picture showing the button pockets on the 3D printed case and the purple PCB underneath](../images/button-pockets.jpg)

## Electronics assembly

Before do anything, plug the Teensy to a computer via its micro USB port and make sure the LED blinks, indicating the Teensy is working correctly. You can then take this opportunity to [flash the pd-camera firmware](../README.md#firmware).

> **WARNING**: If you want to be safe, you can now cut off the copper trace pictured below so the Teensy no longer draws power from its micro USB port. This eliminates the risk for the battery power to flow back into your computer if you connect the Teensy to it (in order to e.g. update the firmware).
>
> - If you cut the trace, you'll then need to turn the battery on even when plugging the Teensy's to a computer via micro USB, but it's foolproof. 
> - If you don't cut the bridge **you must always remember to turn the battery off**, as the Teensy will be powered via its micro USB port.
>
> <p align="center"><a href="https://forum.pjrc.com/threads/70030-Teensy-4-1-Cut-pad-for-USB-location"><img title="Trace to cut to prevent micro USB from powering the Teensy (click for source)" src="../images/trace-to-cut.png"></a/</p>  

### Soldering steps

Solder all male headers to the Teensy, including the 5 usb host pins. It helps to place the long headers on a breadboard to make sure the pins are straight (but you won't be able to do that with the USB Host pins because they're not aliged with the GPIO pins, so for these you can use the project's custom PCB).

Solder the 2 resistors and the toggle switch to the PCB.

Fit the Teensy flush onto the PCB, solder all pins and then cut off the protruding pins underneath, as short as possible.

<img title="Cutting Teensy pins flush" src="../images/hw_cutting-pins.jpg" style="max-height:400px">

Cut 2 female header bars to 9 pins each and solder them to the PCB to create a makeshift connector for the OV7670 module.

<img title="Cutting female headers" src="../images/hw_headers.jpg" style="max-height:400px"> <img title="Camera module connector" src="../images/hw_camera-connector.jpg" style="max-height:400px">

Cut the angled USB C cable at about 25cm and discard the USB A connector. Strip and solder the 4 USB wires to the PCB (*usually*, red for +5v, white for D-, green for D+, and black for GND).

<img title="Soldering the USB Host cable" src="../images/hw_usb-cable.jpg" style="max-height:400px">

Solder BATT +5v and GND from the main PCB to the +5 and GND of the power circuit PCB with short wires, and finally solder the + and - battery pads of the power circuit to your LiPo battery. You can also do (or redo) this step after assembling the case in order to optimize your wiring.

<img title="Soldering the power circuit" src="../images/hw_power.jpg" style="max-height:400px">

Almost done! Time to sideload the Playdate app (from [this repository](https://www.github.com/t0mg/pd-camera-app)) and test your circuit 🙂

Once you're confident with your solder joints, put your cutting pliers at work and make sure nothing protrudes underneath by more than a millimeter or so.

## 3D printed case

The case has 4 parts (body, chin, cover, camera cap) plus an optional flat spring that can help make the USB cable pop out of the chin. It was designed with OnShape (and Freecad orignally but I had to migrate).

Many iterations went into this design. The whole case could be more compact, especially with a smaller battery and fewer features. For example, without the selfie system the chin part could be reduced or removed, and the USB cable replaced with a simple dock connector. However I decided to prioritize versatility and ergonomics over compactness here.

<img title="Exploded view from the OnShape project showing all the parts" src="../images/3d-model-exp.png" style="max-height:600px">

### Printing instructions

You can find the STL files [here](/hardware/case/). All the parts can be printed in one go on a Prusa Mk3S, in PETG at 0.2mm Speed mode, default settings. They were designed to print without support if you lay each part on its flat face. These are external facing sides that take advantage of a textured print bed for a better finish.

Printing takes about 4.5 hours and 61g of filament in total.

<img title="Screenshot of all the parts in Prusa Slicer" src="../images/slicer.png" style="max-height:400px">

<img title="Yes, I know, that print is not from the same Slicer project as above. It is from an older version." src="../images/hw_3d-print.jpg" style="max-height:400px">

### Editing the project files

Should you want to make changes, the entire source project is available publicly in OnShape [here](https://cad.onshape.com/documents/e21c7f87f60d07934982913d/w/b97fe7733e7e7384ebf3b9fe/e/61bb6eb626a207e4d2b47202).

## Putting it all together

The assembly doesn't require any screw or glue, and is fully reversible (mostly because during development, that was really useful).

### Installing the PCB

The PCB slides in and snaps without the need of any tool but it is a tight fit and requires a precise gesture and a bit of force. But don't be scared, the case is sturdy and my PETGs print survived way more assemblies and disassemblies than you should ever need to perform.

Start with the top of the PCB, fitting the little power switch through its hole in the case. Make sure the PCB is flush to the left side of the case, then lower its bottom part slowly. You'll now need to align the Teensy's USB port to the hole in the case on the right. Once it's aligned, push the PCB to the right so it gets in by a millimeter or so. Now press firmly on the PCB to snap it past the tiny leg on the right between USB and power wires, and then the snap lock on the bottom left. It's quite tight, but you can flex the edge of the case slightly outwards near the snap lock while pressing to help the PCB make it through.

### Power circuit

Slide the power circuit in place at a 45 degree angle. Insert the female USB C connector into its socket, then press the back of the board to lock it in place. It should be horizontal.

### USB Cable and chin

The male USB C connector goes through a dedicated hole. Keep the cable loose at this stage.

<img title="Passing the USB cable through its hole in the case" src="../images/hw_cable-through-hole.jpg" style="max-height:400px">

Flip the case and take the chin piece and your Playdate. Connect it, align it with the chin and work backwards to set the length of the cable.

<img title="Locking the cable in the chin" src="../images/hw_cable-in-chin.jpg" style="max-height:400px">

You can add the optional spring part by just sliding it into the chin, this little piece helps when the cable is a bit too stiff to pop out naturally, but makes the "cover mode" a bit more difficult. Either way you can easily disassemble the case to add or remove this part later.

Snap fit the chin part into the main body part then flip it back and secure the remaining part of the USB cable.

### Battery

Now is the time to secure the battery and tidy all your wires inside the case. The 5000mAh battery fits (barely) under the Teensy, with its edges touching the Teensy and the right side of the connector of the camera module (as pictured [above](#usb-cable-and-chin)). 

### Cover

To put the cover, there are 3 snap fit locks to work with. First align the top one, then the right one. Finally flex the cover and push firmly on the left snap lock. To reopen the case you'd insert a spatula, blade or flat screw driver above the left snap lock and use it as a lever to free it.

<img title="Cover in place with camera modules ready to go" src="../images/hw_cover.jpg" style="max-height:400px">

### Camera module and camera cap

You can insert the OV7670 Camera module now.

The camera cap part is also press fit. It comes off easily on purpose, so you can swap the camera module if you have different lens mounts, without having to remove the cover part. Otherwise, you can add a bit of super glue to attach the camera cap to the cover plate permanently.

There is currenlty no cap for larger mounts/lenses such as CS one in the photo above. I don't see this as a major issue as the sensor board holds itself just fine. But for polish reasons, in future updates we might add new caps with stronger fit (eg stap fit joints) and more opening sizes.

### Magnets

Press fit the eight neodymium magnets into their respective holes. I found that it helps to stack 2 of them to align with the hole and initiate a good vertical fit, then to use the handle of a fork or spoon to push the magnet all the way in. If they feel loose  you can add a drop of superglue to secure them (I never had to).

Should you need to reuse these magnets, the case is designed so that you can remove them, by pushing them from the back with a thin screwdriver for the 4 on the chin side, and by pulling them out with small pliers (and cover removed) for the 4 on the back side.

### Pat yourself in the back

Congratulation, you've built your own PD-Camera! 😊
