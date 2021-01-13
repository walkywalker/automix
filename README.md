# Automix

Welcome to Automix, an automated mixing/DJing program. Own electronic music? Prefer listening to a continous mix rather than individual tracks? If so, Automix could be useful to you.

Automix takes a directory containing audio files as input and outputs a MP3 file containing a continous mix of the input tracks, users can control the charachteristics of the output mix by passing in various arguements. Currently this is biased towards a mixing style popular for DnB/Jungle/Breakbeat music.

Refer to the [developer documentation](developer.rst) for a detailed description of the operation of Automix.

## Getting Started

Currently only 64 bit Linux machines are supported. Also only mp3 and m4a format input and output files are supported. The following guide has been tested using Ubuntu 20.04.

### Prerequisites

The following software is required to build and run Automix:

* git
* make
* cmake
* g++ (At least version 8)
* libpthread
* libavutil
* libavformat
* libavcodec

These can easily be installed using a package manager, for example `apt`:

``` bash
> sudo apt update
> sudo apt install git make cmake g++ libavformat-dev
```

### Building
Clone the project and it's submodules, then use make to build it:

``` bash
> git clone https://github.com/walkywalker/automix --recurse-submodules
> cd automix
> make
```
### Running

Automix requires the environment variable `AUTOMIX_HOME` to be set. The simplest method is to set it as the top-level directory of the repository. However it can be set to any directory, as long as `$AUTOMIX_HOME/log` and  `$AUTOMIX_HOME/tmp` exist.

The Automix binary will be in the top-level directory after building, it can be run using:

``` bash
> ./automix -i <path of input directory> -o <path of output file> -m
```
Note the output file must have the `.mp3` extension. The `-m` argument enables multi-threading for track analysis. Information on the other arguments can be accessed by running `./automix -h`. Or, for more detailed descriptions, refer to the [developer documentation](developer.rst).

FFMPEG outputs information to STDERR, therefore it is usually most useful to forward STDOUT to a file.

## Authors

* **[Matthew Walker](https://github.com/walkywalker)** (matthew@ncdconsulting.net) - *Initial work*

## Contributing

Contributions welcome, refer to the [developer documentation](developer.rst) for a detailed description of the state of the project. Feel free to contact me via email to discuss changes.
## License

This project is licensed under the GNU GPLv2 License - see the [COPYING](COPYING) file for details.

Note this project contains modified source code from QM Vamp Plugins, which are also licensed under GNU GPLv2.

## Acknowledgments

### Submodules
* [qm-dsp](https://github.com/c4dm/qm-dsp) (Queen Mary University DSP)
* [fidlib](https://uazu.net/fidlib/) (IIR/FIR filters)
* [Secret Rabbit Code](http://www.mega-nerd.com/SRC/) (Sample rate conversion)
* [pugixml](https://pugixml.org/) (XML processing)
* [Vamp Plugins](https://www.vamp-plugins.org) (Audio plugin API)
### Inspiration
* [QM Vamp Plugins](https://vamp-plugins.org/plugin-doc/qm-vamp-plugins.html) (Audio feature extraction plugins)
* [Mixxx](https://mixxx.org/) (DJ software with MIDI support)




