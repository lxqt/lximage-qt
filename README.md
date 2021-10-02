# LXImage-Qt

## Overview

LXImage-Qt is the Qt port of LXImage, a simple and fast image viewer.

![LXImage-qt](lximage-qt.png)


## Features

* Zoom, rotate, flip and resize images
* Slideshow
* Thumbnail bar (left, top or bottom); different thumbnail sizes
* Exif data bar
* Inline image renaming
* Custom shortcuts
* Image annotations (arrow, rectangle, circle, numbers)
* Recent files
* Upload images (Imgur, ImgBB)
* Take screenshots

More features can be found when it is used. LXImage-Qt is maintained by the LXQt project
but can be used independently from this desktop environment.

## Installation

### Compiling source code

Runtime dependencies are qtx11extras and [libfm-qt](https://github.com/lxqt/libfm-qt)
(LXImage-Qt used to depend on [PCManFM-Qt](https://github.com/lxqt/pcmanfm-qt)
but the relevant code belongs to what was outsourced in libfm-qt).
Additional build dependencies are CMake and optionally Git to pull latest VCS
checkouts.

Code configuration is handled by CMake. CMake variable `CMAKE_INSTALL_PREFIX`
has to be set to `/usr` on most operating systems.

To build run `make`, to install `make install` which accepts variable `DESTDIR`
as usual.

### Binary packages

Official binary packages are available in all major distributions. Just use the distributions'
package manager to search for string 'lximage-qt'.


### Translation

Translations can be done in [LXQt-Weblate](https://translate.lxqt-project.org/projects/lxqt-desktop/lximage-qt/).

<a href="https://translate.lxqt-project.org/projects/lxqt-desktop/lximage-qt/">
<img src="https://translate.lxqt-project.org/widgets/lxqt-desktop/-/lximage-qt/multi-auto.svg" alt="Stato traduzione" />
</a>
