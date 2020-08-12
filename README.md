# The backstory

I've been fooling around with SW-only 3D graphics
[for quite some time](https://www.thanassis.space/renderer.html).
A couple of years ago, [I ported the main logic](https://github.com/ttsiodras/3D-on-an-ATmega328p/)
into an ATmega328P microcontroller, implementing "points-only" 3D rendering,
and driving an OLED display via the SPI interface...  at the
magnificent resolution of 128x64 :-)

<center>
<a href="https://youtu.be/nsqmnkfZtSw" target="_blank">
<img src="contrib/3DFX.jpg">
</a>
</center>

# The challenge

So the path to even more useless tinkering was clear:

**I had to make this work for my ZX Spectrum 48K! :-)**

And... I did:

<center>
<a href="https://youtu.be/Zdb0Jd97Hzw" target="_blank">
<img src="contrib/speccy3d.jpg">
</a>
</center>

Since the Speccy's brain is even tinier than the ATmega328P's, 
I had to take even more liberties:  I changed the computation
loop to orbit the viewpoint (instead of rotating the statue),
thus leading to the simplest possible equations:

    int wxnew = points[i][0]-mcos;
    int x = 128 + ((points[i][1]+msin)/wxnew);
    int y = 96 - (points[i][2]/wxnew);

No multiplications, no shifts; just two divisions, and a 
few additions/subtractions.

But that was not the end - if one is to reminisce, one must go **all the way**.
So after 4 decades, I re-wrote Z80 assembly - and [made much better use](https://github.com/ttsiodras/3D-on-a-ZX-Spectrum-48K/blob/master/statue.c#L46) of the Z80 registers [than any C compiler can](https://retrocomputing.stackexchange.com/questions/6095/).

The result? Almost a 2x speedup, reaching the phenomenal speed of 9.5 frames per sec :-)

# Pre-calculating

I was also curious about precalculating the entire paths and the
screen memory writes - you can see that code in the
[precompute](https://github.com/ttsiodras/3D-on-a-ZX-Spectrum-48K/tree/precompute)
branch.

This version runs 4 times faster (38 frames per sec). In addition,
since I had all the time in the world to precompute, I used the
complete equations ([for rotating the statue and 3D projecting](https://github.com/ttsiodras/3D-on-a-ZX-Spectrum-48K/blob/precompute/statue.c#L42))
in 8.8 fixed-point arithmetic:

<center>
<img src="https://raw.githubusercontent.com/ttsiodras/3D-on-a-ZX-Spectrum-48K/precompute/contrib/linear_algebra.png">
</center>

The reason for the insane speed, is that I precompute the target pixels'
video RAM locations and pixel offsets, leaving almost nothing for the
[final inner loop](https://github.com/ttsiodras/3D-on-a-ZX-Spectrum-48K/blob/precompute/statue.c#L172),
except extracting the memory access coordinates from 16 bits/pixel:

- The offset within the 6K of video RAM, in the upper 13 bits
- The pixel (0-7) within that byte, in the lower 3 bits

It's also worth noting that the [inline assembly version of the "blitter"](https://github.com/ttsiodras/3D-on-a-ZX-Spectrum-48K/blob/precompute/statue.c#L91) is 3.5 times faster than [the C version](https://github.com/ttsiodras/3D-on-a-ZX-Spectrum-48K/blob/precompute/statue.c#L168). Since these are just reads, shifts and writes, I confess I did not expect to see that much of a difference... But clearly, C compilers for the Z80 need all the help they can get :-)

# Next steps

Now all I need to do is wait for my retirement... so I can use
my electronics knowledge to revive my Speccy, and test this code
on the real thing, not just on the Free Unix Spectrum Emulator :-)
