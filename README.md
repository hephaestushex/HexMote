# HexMote
A wireless game controller with a built in IMU, allowing for motion controlled games. Uses the nRF52840 SuperMini and the ICM-20948 for motion controls.

![Animated Gif of HexMote, red](https://github.com/hephaestushex/HexMote/blob/cfdedfd1c7839106c10d388ec9ba267a18893311/assets/Case%20Render.gif)

![Render of HexMote, Front, blue](https://github.com/hephaestushex/HexMote/blob/af47827ae360967722cf17a6e79911b3e41a6037/assets/Blue%20Front%20View.png)

![Render of HexMote, Front, red](https://github.com/hephaestushex/HexMote/blob/af47827ae360967722cf17a6e79911b3e41a6037/assets/Red%20Front%20View.png)

![Render of HexMote, Top Down, red](https://github.com/hephaestushex/HexMote/blob/af47827ae360967722cf17a6e79911b3e41a6037/assets/Red%20Top%20Down.png)

![Render of HexMote, Side, blue](https://github.com/hephaestushex/HexMote/blob/af47827ae360967722cf17a6e79911b3e41a6037/assets/Blue%20Side%20View.png)

# Inspiration
Recently, I started playing Mario Kart with my sister, but unfortunately for her, she's not too good at video games. The controllers I got, while having hall effect joysticks, lacked motion controls, so my poor sister struggled to steer with the tiny joystick, while turning the controller to no avail. So, I decided to make a Wii-mote-esque controller that would improve the usage while playing motion controlled games. That led birth to the HexMote, a wireless game controller built solely to improve less video-game inclined people.

# Schematic + PCB

<img width="864" height="611" alt="image" src="https://github.com/user-attachments/assets/b2598ec7-edc5-4fe6-b978-813494b180e8" />

The schematic initially used a switch matrix, but falling back to direct pins proved better for pcb traces.

<img width="1179" height="655" alt="image" src="https://github.com/user-attachments/assets/37e345e0-645b-4d0c-a873-c05d3ed04780" />

<img width="902" height="432" alt="image" src="https://github.com/user-attachments/assets/7e5aa81d-d149-4442-b008-0b4fe7e5c10d" />

<img width="842" height="413" alt="image" src="https://github.com/user-attachments/assets/64f4e111-49b3-4f3a-8013-0c4fc6f70d35" />

The PCB is 90x40mm, and has surfacemounted THT components through plugging holes with solder, and THT buttons and joysticks

# CAD

The case is split up into 3 parts, one of the top shell, one of the bottom shell, and one of the buttons all together

<img width="645" height="436" alt="Screenshot 2026-07-08 024119" src="https://github.com/user-attachments/assets/4a926f8a-7e00-49a6-b679-12ef7eda1d8a" />

<img width="520" height="643" alt="Screenshot 2026-07-07 191004" src="https://github.com/user-attachments/assets/abce61a6-d118-4a8d-94ee-f7643fd23a62" />

The PCB mounts on printed standoffs with heatset inserts placed within them, and the m3x6 screws directly into the top shell, through the pcb, and into the insert

# Firmware

# BOM

| Item Name | Qty | Price (total) (USD) | Item link | Notes |
|---|---|---|---|---|
| nRF52840 ProMicro | 1 | $0.99 | [Link](https://www.aliexpress.com/item/1005007383270623.html) | Any nice!nano compatible board should work, check pcb footprint though |
| ICM-20948 Breakout Board | 1 | $4.20 | [Link](https://www.aliexpress.com/item/1005009307235033.html) | You can get a different IMU, but you have to code the sensor fusion code yourself |
| Right Angle Buttons | 2 | $0.99 | [Link](https://www.aliexpress.com/item/1005010093330224.html) | Make sure you get the right length (6x6x7) |
| JST-PH Connector | 1 | $0.99 | [Link](https://www.aliexpress.com/item/1005008970428474.html) | Make sure you get the right connector (right angle) |
| ALPS RKJXV Joystick | 1 | $0.99 | [Link](https://www.aliexpress.com/item/1005009112250803.html) | Make sure you get the PS4 style joystick |
| Soft Tactile Pushbutton | 8 | $0.99 | [Link](https://www.aliexpress.com/item/1005004588874318.html) | Do not substitute for regular clicky buttons, they are not the same footprint |
| M3x5x4 Inserts | 4 | $0.99 | [Link](https://www.aliexpress.com/item/1005011959188413.html) | Ships as 100 |
| M3x6 screws | 4 | $0.99 | [Link](https://www.aliexpress.com/item/32794842281.html) | Ships as 100 |
| 3.7v 500 mah lipo | 1 | $0.99 | [Link](https://www.aliexpress.com/item/1005001485127987) | Really anything works here so long as it meets the 5mm thick, up to 35mm by 80mm dimension requirement and has a jst ph 2.0 connector |
| PCB | 1 | $3.50 | [Link](https://github.com/hephaestushex/HexMote/blob/2a16dff57269423145945d0d09d0ee5307a6468b/pcb/production/hexmote_pcb.zip) | Link is to gerber files, JLC ships the total for 3.50 to Canada |
| Case | 1 | $1.00 | [Link](https://github.com/hephaestushex/HexMote/blob/d6d5b4342bca9d7315f2470e53cb6bb6a80dd431/cad/3d%20print.3mf) | Price differs based on filament. Bambu Lab PLA Matte is used here |
| **Total Price (USD):** | | **$16.62** | | |





