C Framebuffer display of a remote JPEG image / Octoprint monitoring from REST API
=====

Small C program which is able to gather a remote JPEG image at regular intervall (via HTTP / `curl` library) and display it fullscreen onto a framebuffer device (via [fbg](https://github.com/grz0zrg/fbg) library) along with informations gathered from a REST API (via [ulfius](https://github.com/babelouest/ulfius) library)

This was made to monitor my 3D printer (with [octoprint](https://octoprint.org/) webcam stream & API) from my desk using a NanoPI NEO 2  and a small LCD display and thus allow to occasionally stream the framebuffer content to a streaming service like [twitch](https://www.twitch.tv/).

Features :

* Octoprint stream status (red / green); if down it mean that either the physical connection to octoprint was lost or that the octoprint device had an issue.
* Printer power / octoprint serial status (ON / OFF)
* Realtime statistics (temperature, host board temperature, current and last job, time, job progression bar)
* Realtime tool/bed temperature graphics

This program can be easily configured.

Altough this was made for octoprint, it has all the basic tools to be adapted for all sort of embedded monitoring needs...

To compile it, you will need my [framebuffer graphics library](https://github.com/grz0zrg/fbg), libcurl and [ulfius](https://github.com/babelouest/ulfius).

**Note** : Linux framebuffer streaming example (with twitch service) :

```
ffmpeg -f fbdev -framerate 25 -i /dev/fb0 -c:v libx264 -preset veryfast -maxrate 2000k -bufsize 4000k -vf "format=yuv420p" -g 50 -f flv rtmp://live.twitch.tv/app/<stream key>
```

## Screenshots

![Prusa 3D printer monitoring with octoprint](/screenshot.png?raw=true "Prusa 3D printer monitoring with octoprint")
![Prusa 3D printer monitoring with octoprint](/screenshot2.png?raw=true "Prusa 3D printer monitoring with octoprint")
![Prusa 3D printer monitoring with octoprint](/screenshot3.png?raw=true "Prusa 3D printer monitoring with octoprint")

License
=====

BSD, see LICENSE file