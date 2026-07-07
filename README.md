# HexMote
A wireless game controller with a built in IMU, allowing for motion controlled games. Uses the nRF52840

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





