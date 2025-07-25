# TFT and ESP32 Spotify Display

Uses the ST7735 and ESP32 to fetch Spotify listening data and display it. The worker.js file is setup in a cloudflare worker, the esp32 fetches the data and displays it using the Tjpg library. Credit to the worker.js file here: https://github.com/Naushikha/Spotify-Widget. Right now, the colors are really bizarre and the listening info only shows for a few seconds, but il format it all later.

## Features

- Song Title
- Artist info
- Album cover

## Update (7/25/25)

Fixed the weird colors, I was using the INITR_GREENTAB instead of INITR_BLACKTAB. The scrolling logic only scrolls the name when the text is longer than the album cover width; otherwise, it just prints the name once. However, for some reason, when it goes past the left side, it spam prints the name a couple of pixels below the scrolling text. 
