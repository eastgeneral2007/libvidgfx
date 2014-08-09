Libvidgfx
=========

Libvidgfx is a C++ DirectX/OpenGL graphics abstraction library with a simplified interface designed for video compositing. This library is a part of the [Mishira project](http://www.mishira.com).

License
=======

Unless otherwise specified all parts of Libvidgfx are licensed under the standard GNU General Public License v2 or later versions. A copy of this license is available in the file `LICENSE.GPL.txt`.

Any questions regarding the licensing of Libvidgfx can be sent to the authors via their provided contact details.

Version history
===============

A consolidated version history of Libvidgfx can be found in the `CHANGELOG.md` file.

Stability
=========

Libvidgfx's API is currently extremely unstable and can change at any time. The Libvidgfx developers will attempt to increment the version number whenever the API changes but users of the library should be warned that every commit to Libvidgfx could potentially change the API, ABI and overall behaviour of the library.

Building
========

Building Libvidgfx is nearly identical to building the main Mishira application. Detailed instructions for building Mishira can be found in the main Mishira Git repository. Right now development builds of Libvidgfx are compiled entirely within the main Visual Studio solution which is the `Libvidgfx.sln` file in the root of the repository. Please do not upgrade the solution or project files to later Visual Studio versions if asked.

Libvidgfx depends on Qt and Google Test. Instructions for building these dependencies can also be found in the main Mishira Git repository.

Contributing
============

Want to help out on Libvidgfx? We don't stop you! New contributors to the project are always welcome even if it's only a small bug fix or even if it's just helping spread the word of the project to others. You don't even need to ask; just do it!

More detailed guidelines on how to contribute to the project can be found in the main Mishira Git repository.
