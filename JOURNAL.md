# July 6th

*Note: I was too locked in to take screenshots, but I did record through lapse*

[Recording 1](https://lapse.hackclub.com/timelapse/f-8UHHHKltJs)
[Recording 2](https://lapse.hackclub.com/timelapse/fGfNaLuvpSXH)


## 6h

I had a vague idea of what I was building before I went into this, so here's a quick explanation:

 - Wireless Game Controller with Motion Controls
 - uses nRF52840 for power efficient BLE (seeed studio xiao nrf52840 board)
 - Uses 18650 cell
 - has low profile joystick, 4 a/b/x/y buttons, +/-, l/r
 - has an IMU for motion controls (generic arduino module?)

From here, I just went on a long speedrun of pcb design. It mostly went as you expected (place symbols, wire it up, assign footprints, arrange, wire, done!), but I ran into several hurdles along the way. I'll explain each hurdle in detail

### Parts Selection

There wasn't enough pins on the XIAO, so I opted for the nRF52840 SuperMini board. It's a clone of the nice!nano, but its readily available on aliexpress

The Wii uses an IR sensor for detecting heading, so I needed a board with heading detection, so no mpu6050. I found the ICM-20948, which has 9 DOF (3x accelerometer, 3x gyro, 3x magnetometer)

The 18650 is too big and would make the overall controller large and heavy, so I switched to a 3.7v LiPo Battery.

None of the low-profile joysticks seem to have a basic pinout on aliexpress, so I opted for a ALPS Joystick that you would find in a PS4/XBOX controller. It should be smaller than the arduino joystick modules. At least it will be mounted directly to the pcb to save on vertical clearance. 

The rest of the parts selection was easy, simply searching on aliexpress for something that'll work.

### PCB Footprints/Schematics

There aren't any readily available footprints/schematics for the joystick, so I had to scour the internet for one that'll work with the joystick. I found this module from [github](https://github.com/little-scale/PS4_joystick_footprint), and the person also made it in real life so I trusted it would fit.

<img width="389" height="382" alt="image" src="https://github.com/user-attachments/assets/06769783-7f28-437f-96a4-aa4bdd45d873" />

<img width="415" height="533" alt="image" src="https://github.com/user-attachments/assets/3dab6d00-18d2-4408-9e61-6c668dffdf9b" />


I didn't bother with finding footprints for the squishy buttons (IMO better than clicky for game controllers), so I made a footprint based on the aliexpress diagram.

<img width="640" height="352" alt="image" src="https://github.com/user-attachments/assets/230cf374-f007-4ff8-89d1-ce6227ba72df" />

As for the IMU, I had to create a footprint to ensure my parts won't intrude on it. I also realized I was looking at another module halfway into designing the PCB.

<img width="1004" height="629" alt="image" src="https://github.com/user-attachments/assets/d2afc864-d6de-46cf-9466-396afc1f0279" />


### IMU Mounting

I can't place the imu off to the side for two reasons. A, it would take too much room, and B, it wouldn't record motions properly

I chose to mount it on the back to solve these problems, but the long headers would clash with the MCU. I know of a method to solder tht boards smd style, by plugging the holes with solder, so I changed the THT pads to SMD pads for it.

I also found out that the MCU might mess with the magnetometer, so I placed it down the PCB to avoid it

<img width="1515" height="725" alt="image" src="https://github.com/user-attachments/assets/176a8c66-bcf4-4fdf-8465-603685ae685f" />

### To make a matrix, or not to make a matrix, that is the question

I initially used a switch matrix to save on pins, but the diodes couldn't be placed nicely without taking more space. The wiring turned out horrible with way too much vias. It could also be because I didn't route traces properly, but direct pins plus a ground plane made the wiring way cleaner.

<img width="792" height="432" alt="image" src="https://github.com/user-attachments/assets/054813bd-758b-49e3-a82a-a4b511a11527" />

### Layout

The PCB layout was tricky, as I was rusty, plus there's a lot more components. In the end, I managed to get it squished under 90mm by 40mm, so $2 dollar pcbs for JLC!

### nRF52840 internal multiplexer my goat

I would have been fried if the nRF52840 kept its pins static. Luckily, almost every pin can be used for everything, so I could select pins based on which was closer and cleaner.

### Conclusion

Well I finished the PCB, so heres a few pictures of it.

<img width="1179" height="655" alt="Screenshot 2026-07-06 212102" src="https://github.com/user-attachments/assets/f3b4bb97-130c-48b2-a14b-6089fd5bfef2" />

<img width="902" height="432" alt="Screenshot 2026-07-06 212216" src="https://github.com/user-attachments/assets/dcdbb81f-552d-437e-9464-fcea4afaae79" />

<img width="842" height="413" alt="Screenshot 2026-07-06 212236" src="https://github.com/user-attachments/assets/6f354638-b86e-4d5b-ab4f-e669f6b7db3f" />

Oh I did run into a couple of DRC errors. It had to do with the ground plane, but all I had to do is move a couple of traces around.

Also Claude is the GOAT of AI, it can actually do stuff without hallucinating (chatgpt), or run into safety filter every 5 seconds (gemini), and it even has a better toolbox! Would not have done it this fast without claude.

# July 7th-8th (not really, i just pulled an all nighter)
## 7h (this much screen time can't be good on my eyes)

[Timelapse](https://lapse.hackclub.com/timelapse/uQqbto0yiFiv) please look at this as I was too locked in to take screenshots apart for a few.

So I didn't do any CAD yet, so I cracked down on it and started in the evening, and stayed up until 4:30? (its 4:20 as of writing this)

First issue was I was rusty, so apart of creating the base and sticking a couple of pegs on it, I was stuck on figuring out how to create the holes properly

<img width="520" height="643" alt="Screenshot 2026-07-07 191004" src="https://github.com/user-attachments/assets/05f18a0b-72ba-4456-910e-ef34a02207ac" />

I ran into a lot of unconstrained lines, but after a while I gave up, considering it doesn't necessarily break anything at this scale (note to self: take an actual CAD course). That doesn't mean I didn't try, I just didn't try too hard to fix the annoying constraints (text boxes) 

Another annoying thing is finding CAD reliably without making 5000 accounts. I did make a handheld a year back, so I lowkey just stole the cad files from that since I couldn't find it today. It's also today I resigned myself to the fact that I will not get this in time for Outpost (sadge) but redesigning it to house more expensive parts that might arrive on time didn't make sense. Plus no one sells lipos and nice!nanos for a decent rate in Canada.

It also occured to me that I didn't account for lipos being wired wrong, so I added solder pads to fix that. It did appear that most had + on the right, but I figured this would be better. That also led into a rabbit hole of finding lipos that would work, but after chatting with Claude (goat btw), I figured anything about 500mah that was legit would work. I also made sure to design the case to fit the lipo (anything 5mm thick, within 40x90mm). In the end, I just slapped some aliexpress links there that would work.

CAD was not fun, so here are some highlights (or lack thereof):

### Creating holes

I had no idea how to make the holes in the right place, so I bounced back and forth between the PCB and Fusion measuring out dimensions and seeing if they worked. I realized too late I can import an stl into a part studio. After a lot of trial and error, I created the holes in the right place for the shoulder buttons and the USB-C slot. This time, I actually spent time and made the hole look pretty, and I think it was worth it!

<img width="880" height="260" alt="image" src="https://github.com/user-attachments/assets/c584b19a-082c-47c5-a0aa-65f6c0432848" />

### Create elevated portion for Joystick

So like I said, I don't really know CAD too well, just enough to trudge along. So when it came time for me to do something other than a sketch and extrude, I blanked out. After some searching up, I learned how to create an offset plane. From there, it wasn't too bad, just a matter of creating curves and revolving them.

<img width="712" height="481" alt="image" src="https://github.com/user-attachments/assets/341a8db6-7cce-47e5-b229-1c39dcede56e" />


### How to fit everything together without making it too thick

So i really should have went into this with a written out plan. I figured I could have two circles of inserts, one for the pcb, and one for the case, but that was too thick, so I reduced it to one by sandwiching the whole case+pcb under one screw and insert.

<img width="665" height="539" alt="image" src="https://github.com/user-attachments/assets/2471c04b-383b-46ed-ac90-787018921913" />

### Buttons

This actually wasn't hard as it was tedious, creating circles and making sure the buttons remained separate from the case, which happened a few times

<img width="603" height="409" alt="image" src="https://github.com/user-attachments/assets/6a0f6eba-406c-4e80-b19a-5efec2fdf920" />

<img width="305" height="247" alt="image" src="https://github.com/user-attachments/assets/e7241518-28fa-4243-b8f2-55b617174540" />


So that's an issue, but I figured that it shouldn't matter too much, as no one is necessarily needing to press - while controlling the joystick in the horizontal position, and in the vertical position, it can be held easily with the right hand or dual handed like a sword.

### Text
Text is a pain to constrain, I'll leave it at that. I didn't even bother doing it properly

### Shoulder Buttons

Initially, I planned to leave the shoulder buttons alone, but I ended up designing them

<img width="609" height="192" alt="image" src="https://github.com/user-attachments/assets/bf5a640d-8e4f-40bf-a3f0-82b77f0bc4bd" />


So I decided to design shoulder buttons for two reasons:
-  They're probably annoying to reach and press
-  they kind of look like nipples

<img width="583" height="184" alt="image" src="https://github.com/user-attachments/assets/d5da6491-f951-4dbd-91ee-5cc265d5908a" />

I had to make sure it doesn't look too bad, so I designed the guard rails as well.

### Rendering
I wanted to render it, but apparently, being linked forces all of the objects to stay the same colour, but fusion removed the break link feature for some reason, causing me to crash out for 10 minutes. (claude my goat for helping me break the link through another menu). Eventually, I got it, set it to totally not important colours, and rendered them!

<img width="1520" height="572" alt="Red Top Down" src="https://github.com/user-attachments/assets/78eb8e7a-3ff8-41ee-adf5-05aa129e0718" />
<img width="1520" height="572" alt="Red Front View" src="https://github.com/user-attachments/assets/c16003ec-84dd-48e2-965e-5cea1e2b5f66" />
<img width="1520" height="572" alt="Blue Side View" src="https://github.com/user-attachments/assets/ad403a77-4959-400a-b91b-056692164d8a" />
<img width="1520" height="572" alt="Blue Front View" src="https://github.com/user-attachments/assets/89cb149a-e461-4554-8691-a74068d2227b" />

### Conclusion

It is now 5 AM, and I have been up for nearly 23 hours. I doubt I can work on firmware now, so I'll come back to it tomorrow. No actually today.

Overall a pretty productive day(s) I would say.








