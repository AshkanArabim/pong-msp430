# Pong for MSP430G2

This is a simple pong game I've made for my computer organization class. 

Features include: 
- Smooth graphics due to using interrupts for logic (Hence, no bottleneck caused by graphics).
- A scoreboard.
- Avoidance of the S2 input button (because it acted weird in my experience).

Interested in this project? Fork it and make it better!

## Demo

<img src="https://github.com/AshkanArabim/pong-msp430/assets/71609332/39c3977c-0c54-4f59-a660-fb9898c2844a" alt="msp_pong_pic" width="400"/>

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

## Acknowledgements
Credits to my professor, [Eric Freudenthal](https://github.com/robustUTEP) for the platform-specific low-level libraries and configurations.
