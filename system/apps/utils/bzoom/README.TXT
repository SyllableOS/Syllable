BZoom v1.0                        http://www.shell.linux.se/digiz/atheos/
==========

Copyright (C)2002 digitaliz

BZoom is a pixel zoomer for AtheOS. It is released under the GNU General
Public License. See the file COPYING for more information.

BZoom is based on AZoom by Chir. The B stands for "Better"... :)

Now, you probably wonder what makes it better, right?
Ok, here are the differences from AZoom:

+ Faster zooming, using a bitmap scaling algorithm
+ Shorter update interval (50 ms => 25 Hz)
+ Longer update interval when the mouse pointer is not moved (750 ms)
+ Zooms in more while the right mouse button is held down
+ Works in both 15/16 and 32 bit modes.
  Should work in 8 and 24 bit too (not tested).
- If the screen mode changes, BZoom will have to be restarted
  (also meaning you can't move it between desktops). I think I know how
  to fix this, but right now I have more important projects to deal
  with.

  + = :)
  - = :(

The reason why I made these changes to AZoom is that I needed to do
some quick tests of the zooming algorithm, which I will use in a future
project; APaint.
Chir did not have any time to work on AZoom right now, so I decided to
release this as a separate program. When he resumes working on it, and
if he chooses to use my zooming stuff, I'll probably stop developing
BZoom. No point in having to progs with the same code.

Ok, time to stop wasting time writing stuff that nobody ever reads. :)
Let's move on to the important issue:

Installation
============

The easy way:

 * Copy bzoom somewhere, your /system/bin/ directory, for example.

The "hard" way:  =)

 make
 make install

BZoom is installed by default into /Applications/BZoom.

Authors
=======

BZoom is mostly written by Chir <chir@geocities.com>
The zooming parts are written by digitaliz <digitaliz@boing.nu>, who
may also be found on ICQ: 11214923.

History
=======

V1.0 2002-01-07
     First Release.

Future
======

Plans for the future:

* Turn it into a plugin for Launcher, as soon as Launcher 1.0 is ready.
  That would be really l33t, wouldn't it? ;)

* Get someone (ie. not me) to optimize the zooming algorithm and
  rewrite it in asm.

* Get Kurt to implement it in the OS (View::DrawBitmap) where it should
  have been already. ;)

* Fix the bugs. (low priority... ;) )

