FullscreenGLApp
=

Built for ML SDK 0.19.0. Include VS 2017 project files, but should build fine with just mabu.

About This Example
-

The primary purpose of this example is:

- To illustrate how to set up OpenGL transforms to draw objects aligned with the real world in a fullscreen immersive application on the Magic Leap headset using the C API.

This example also illustrates the following:

- Plane Tracking: Querying for rectangular planes in the world mesh.
- Head Tracking: Retrieving the headset position and orientation.
- Input: Polling the controller and responding to button presses.
- Logging: Use of tags to identify the application.
- GLM: Use of GLM for basic matrix math.

To keep things simple, shortcuts were taken, and this should *not* serve as an example of:

- Notifying the user of an error.
- Resource cleanup after an initialization error.

Other notes:

- GLAD is used for both host and device builds.
- OpenGL 3.0 Compatibility Profile is used, but the same concepts hold for Core Profile.

Running The Example
-

When the program is run it will display detected planes as rectangles with a grid on them. Planes tagged as "ceiling" will be blue, planes tagged as "floor" will be red, all others will be white. The plane normal is draw as a yellow/white line from the center of the plane.

Plane detection is done in real time in a 15 meter cube centered on your current location. If no planes are detected, the display will be red. In this case, look around a bit to scan the environment until planes are detected.

Issues?
-

Please report bugs on the issue tracker here on GitHub.

There is a forum post for this example [here](https://forum.magicleap.com/hc/en-us/community/posts/360040282051-Fullscreen-GL-App-Example).