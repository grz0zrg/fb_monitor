Framebuffer display of a remote JPEG image at regular intervall
=====

Small C program which is able to gather a remote JPEG image at regular intervall (via HTTP / `curl` library) and display it fullscreen onto a framebuffer device (via [fbg](https://github.com/grz0zrg/fbg) library) along with informations

This was made to monitor my 3D printer (from octoprint stream) from my desk using a NanoPI NEO 2  and a small LCD display and occasionally stream the framebuffer content to a streaming service like [twitch](https://www.twitch.tv/).

Also display the board temperature and display connection status as a red / green indicator.

This can be adapted for all sort of needs.

To compile it, you will need my [framebuffer graphics library](https://github.com/grz0zrg/fbg) and libcurl.

Linux framebuffer streaming example (with twitch service) :

```
ffmpeg -f fbdev -framerate 25 -i /dev/fb0 -c:v libx264 -preset veryfast -maxrate 2000k -bufsize 4000k -vf "format=yuv420p" -g 50 -f flv rtmp://live.twitch.tv/app/<stream key>
```

## Screenshots

![Prusa 3D printer monitoring with octoprint](/screenshot.png?raw=true "Prusa 3D printer monitoring with octoprint")

License
=====

BSD, see LICENSE file