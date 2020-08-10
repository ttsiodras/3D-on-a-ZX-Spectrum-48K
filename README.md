I've been fooling around with SW-only 3D graphics
[for quite some time](https://www.thanassis.space/renderer.html).
A couple of years ago, I ported the main logic into an
ATmega328P microcontroller, implementing "points-only" 3D rendering,
and driving an OLED display via the SPI interface...  at the
magnificent resolution of 128x64 :-)

<center>
<a href="https://youtu.be/nsqmnkfZtSw" target="_blank">
<img src="contrib/3DFX.jpg">
</a>
</center>

So the path to even more useless tinkering was clear: I had to make
this work for my ZX Spectrum 48K! :-)

And I did.

Since the Speccy's brain is even tinier than the ATmega328P's, 
I had to take even more liberies:  I changed the computation
loop to orbit the head around (instead of the statue), thus
leading to the simplest possible equations:

    int wxnew = points[i][0]-mcos;
    int x = 128 + ((points[i][1]+msin)/wxnew);
    int y = 96 - (points[i][2]/wxnew);

No multiplications, no shifts; just two divisions, and a 
few additions/subtractions.

I was also curious about precalculating the entire paths and the
screen memory writes - you can see that code in the "precompute"
branch.

Now all I need to do is wait for my retirement - so I can revive
my Speccy and test this code on it, and not just on FUSE :-)
