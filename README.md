# SynthXEX

Copyright (c) 2024-25 Aiden Isik

Licensed under the GNU AGPLv3 or later, using libraries with various licenses. See [Licensing](#licensing).

## Description

SynthXEX is the XEX(2) builder (and thus the last step in building an executable) for the [OpenXeChain](https://github.com/OpenXeChain) project. OpenXeChain is a free/libre/open-source development toolchain for the Xbox 360, designed to do the same job as the official Xbox 360 SDK (building applications which run on the Xbox 360 OS), without the legal and outdated tech issues which come with that.

This was developed by referencing public knowledge on the XEX(2) file format, along with staring at XEX files in a hex editor to decypher aspects which do not seem to be documented anywhere. No Microsoft code was decompiled to develop SynthXEX, and I ask that any contributors do not do so either.

This is in early development and MANY features are missing (most notable exports). No guarantees are made as to the functionality of the program, or it's stability. Functionality may change at any time.

This *should* support any POSIX-compliant operating system, along with Windows.


## Building (POSIX, standard method)

### Prerequisites

- A C compiler (GCC/Clang)
- CMake (>= 3.24.2)
- Make
- Git

To install these on Debian: ```sudo apt install gcc cmake make git```

### Downloading

Clone: ```git clone https://github.com/OpenXeChain/SynthXEX```

### Compiling

Switch to the newly cloned directory: ```cd SynthXEX```

Make a build directory and switch to it: ```mkdir build && cd build```

Generate build scripts (replacing ```<prefix>``` with where you want to install, e.g. /usr for /usr/bin/synthxex): ```cmake -DCMAKE_INSTALL_PREFIX=<prefix> ..```

Build: ```make```

Install: ```sudo make install```


## Building (Guix)

### Prerequisites

- Guix
- Git

### Compiling

Choose ONE of the options below.

#### Compiling & Installing

Clone: ```git clone https://github.com/OpenXeChain/SynthXEX```

Switch to the newly cloned directory: ```cd SynthXEX```

Install: ```guix time-machine -C channels.scm -- package -f synthxex.scm```

#### Compiling Only

Clone: ```git clone https://github.com/OpenXeChain/SynthXEX```

Switch to the newly cloned directory: ```cd SynthXEX```

Build: ```guix time-machine -C channels.scm -- build -f synthxex.scm```

#### Compiling & Installing to Config

TODO


## Building (Nix)

### Prerequisites

- Nix
- Git

### Compiling

Choose ONE of the options below.

#### Compiling & Installing

Clone: ```git clone https://github.com/OpenXeChain/SynthXEX```

Switch to the newly cloned directory: ```cd SynthXEX```

Run: ```nix profile install .#synthxex```

#### Compiling Only

Clone: ```git clone https://github.com/OpenXeChain/SynthXEX```

Switch to the newly cloned directory: ```cd SynthXEX```

Run: ```nix build .#synthxex```

#### Compiling & Installing to Config

Add SynthXEX to your configuration.nix file:

```
{ config, modulesPath, lib, pkgs, ... }:

let
  synthxex = builtins.getFlake "git+https://github.com/OpenXeChain/SynthXEX";
  synthxexPkgs = synthxex.packages.${pkgs.hostPlatform.system};
in {
  # ...
  environment.systemPackages = with pkgs; [
    # ...
    synthxexPkgs.synthxex
  ];
}
```


## Special thanks to:
- [InvoxiPlayGames](https://github.com/InvoxiPlayGames), for help understanding the XEX format
- [emoose](https://github.com/emoose), for xex1tool, which was very useful for taking apart XEX files and getting info from them
- [Free60Project](https://github.com/Free60Project), for documentation on the XEX format, which was useful in the early days
- [Vali0004](https://github.com/Vali0004), for helping clean up and harden code, along with Nix packaging
- Several members of the Xbox360Hub Discord server, for a multitude of reasons


## Contributing

See [CONTRIBUTING.md](./CONTRIBUTING.md).


## Licensing

### SynthXEX

Copyright (c) 2024-25 Aiden Isik

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

### getopt_port

Copyright (c) 2012-2023, Kim Grasman <kim.grasman@gmail.com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Kim Grasman nor the
  names of contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL KIM GRASMAN BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### GNU Nettle

Copyright (C) 2001, 2013 Niels Möller

This file is part of GNU Nettle.

GNU Nettle is free software: you can redistribute it and/or
modify it under the terms of either:

* the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 3 of the License, or (at your
  option) any later version.

or

* the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your
  option) any later version.

or both in parallel, as here.

GNU Nettle is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received copies of the GNU General Public License and
the GNU Lesser General Public License along with this program. If
not, see http://www.gnu.org/licenses/.
