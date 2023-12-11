# arch1-project3-lcd

This is a simple pong game I've made for my computer organization class. 

Features include: 
- Smooth graphics due to using interrupts for logic. (Hence, no bottleneck caused by graphics)
- A scoreboard
- Avoidance of the S2 input button (because it acted weird in my experience).

Interested in this project? Fork it and make it better!

## Demo
Check out this video from my YouTube channel: https://youtu.be/WTMFzDpH8JE?si=mcIu0uIiT4K9P9I9

## Installation
- Install required C development packages (Linux only)
    - I'll post instructions soon...
    - In the meantime you can try compiling and see what's missing based on the error messages. (Or take a look here: https://www.ti.com/tool/MSP430-GCC-OPENSOURCE)
- Open root directory
- `make`
- `cd ./pong`
- Connect your "MSP430G2 Launchpad" board through USB
- `make load`
- Press your MSP's reset button
- Start playing!

Enjoy!
