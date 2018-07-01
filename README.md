Framebuffer display of a remote JPEG image at regular intervall
=====

Small C program which is able to gather a remote JPEG image at regular intervall (via HTTP / `curl` library) and display it fullscreen onto a framebuffer device (via [fbg](https://github.com/grz0zrg/fbg) library) along with informations

This was made to monitor my 3D printer (from octoprint stream) from my desk using a NanoPI NEO 2  and a small LCD display and occasionally stream the framebuffer content to a streaming service like [twitch](https://www.twitch.tv/).

Also display the board temperature and display connection status as a red / green indicator.

This can be adapted for all sort of needs.

To compile it, you will need my [framebuffer graphics library](https://github.com/grz0zrg/fbg) and libcurl.

License
=====
BSD, see LICENSE file