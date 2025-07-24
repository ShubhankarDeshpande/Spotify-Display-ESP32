# TFT and ESP32 Spotify Display

Uses the ST7735 and ESP32 to fetch Spotify listening data and display it. The worker.js file is setup in a cloudflare worker, the esp32 fetches the data and displays it using the Tjpg library. Credit to the worker.js file here: https://github.com/Naushikha/Spotify-Widget. Right now, the colors are really bizarre and the listening info only shows for a few seconds, but il format it all later.

## Features

- Song Title
- Artist info
- Album cover
