Fork of libwebp that removes the lossy encoder and everything related to it, as
well as YUV input. Structure fields related to these parts have been dummied out
to stay API- and ABI-compatible with the official library.

### Why so violent? Why not just add a build flag?

Lossless WebP is the best widely-supported codec for low-color 90s-era retro
game images in 2025, [beating JPEG XL in the vast majority of my test
cases](https://github.com/nmlgc/ssg/issues/54#issuecomment-2746624195). I want
to understand, use, and maybe even tune the encoder without being constantly
distracted by code and options that only apply to the lossy mode. PNG has
received several alternative encoders over the years, but WebP hasn't received
anywhere close to that level of attention – despite even its lower effort
settings easily outperforming the strongest and slowest PNG encoders. There
seems to be much more potential in lossless WebP than people have tapped into so
far. Maybe because it gets overshadowed by the frequent criticism of its lossy
mode?

Also, binary size when statically linking the library. Since the lossy and
lossless encoders are selected via structure fields rather than having unrelated
API entry points, optimizers have a hard time removing the lossy branches you
never wanted anyway. Removing all this code reduces the compiled binary size by
172.5 KiB in my test case. This might not seem like much these days, but it's 1)
code I did not ask for and 2) code I absolutely do not want to run by accident.

Really though, why have people started to bundle lossless and lossy algorithms
under the same format in the first place, especially if they share a negligible
amount of code? It might make sense for Opus where SILK and CELT are different
kinds of lossy, but *lossless* and lossy are two completely different paradigms.
The bloat and usability confusion far outweigh any [situational tricks this
might
offer](https://www.reddit.com/r/compression/comments/wc61wt/comment/iiaqvbi/).

Original README below.

----

# WebP Codec

```
      __   __  ____  ____  ____
     /  \\/  \/  _ \/  _ )/  _ \
     \       /   __/  _  \   __/
      \__\__/\____/\_____/__/ ____  ___
            / _/ /    \    \ /  _ \/ _/
           /  \_/   / /   \ \   __/  \__
           \____/____/\_____/_____/____/v1.5.0
```

WebP codec is a library to encode and decode images in WebP format. This package
contains the library that can be used in other programs to add WebP support, as
well as the command line tools 'cwebp' and 'dwebp' to compress and decompress
images respectively.

See https://developers.google.com/speed/webp for details on the image format.

The latest source tree is available at
https://chromium.googlesource.com/webm/libwebp

It is released under the same license as the WebM project. See
https://www.webmproject.org/license/software/ or the "COPYING" file for details.
An additional intellectual property rights grant can be found in the file
PATENTS.

## Building

See the [building documentation](doc/building.md).

## Encoding and Decoding Tools

The examples/ directory contains tools to encode and decode images and
animations, view information about WebP images, and more. See the
[tools documentation](doc/tools.md).

## APIs

See the [APIs documentation](doc/api.md), and API usage examples in the
`examples/` directory.

## Bugs

Please report all bugs to the [issue tracker](https://issues.webmproject.org).
For security reports, select 'Security report' from the Template dropdown.

Patches welcome! See [how to contribute](CONTRIBUTING.md).

## Discuss

Email: webp-discuss@webmproject.org

Web: https://groups.google.com/a/webmproject.org/group/webp-discuss
