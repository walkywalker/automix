===============================
Automix Developer Documentation
===============================

This document aims to give an overview of the operation of Automix, along with limitations and scope for further improvements.

Overview of Operation
---------------------

The operation of Automix can be decomposed into 3 phases: Analysis, Mix and Perform.

Analysis
~~~~~~~~

First the list of tracks to analyze is extracted from the input directory. The maximum number of tracks to use is passed in by the `-l` argument (default 25).

The XML file `$AUTOMIX_HOME/tmp/automix.xml` is parsed, if it does not exist is it created. For each track, if there is not an XML element that corresponds to the track, then it is analysed and it's features are extracted. Else the features for that track are read from the XML.

The beat grid of a track is calculated from the output of 2 QM Vamp Plugins, BeatTrack and OnsetDetect. Both plugins are configured to use broadband detection functions, this seems to be more accurate than complex spectral difference most of the time. These plugins output the most likely beat and drum positions. For some interesting reading refer to the papers linked in the `QM Vamp Plugins <https://vamp-plugins.org/plugin-doc/qm-vamp-plugins.html/>`_ , most importantly `Context-Dependent Beat Tracking of Musical Audio <http://www.eecs.qmul.ac.uk/~markp/2007/DaviesPlumbley07-taslp.pdf/>`_ and `Drum Source Separation using Percussive Feature Detection and Spectral Modulation <http://dublinenergylab.dit.ie/media/electricalengineering/documents/danbarry/15.pdf/>`_.

The user can speficy a hint at the input tempo of the tracks using the `-it` argument (default 87.5 BPM). Due to the nature of the techniques used for beat extraction the beats will not be evenly spaced, therefore the most likely fixed-tempo beat grid must be calculated. First the standard deviation of the tempo is extracted from the beat positions, the constant tempo is set as the mean of the tempos within the standard deviation, rounded to the nearest 0.25 BPM. This rounding is unfortunately necessary but most electronic music will have a tempo that is a multpile of 0.25 BPM.

Now the tempo of the beat grid is known, it must be aligned. The beat positions are processed to find a window of 40 beats that are all within 2 standard deviations of the mean tempo. A fixed beat grid is then shifted across these positions until the minimum distance between the positions and the fixed grid is found. This is our first guess at the beat grid.

Unfortunatly, depending on the drum pattern in the section used to extract the grid, the beat grid can be off-beat by some fraction of a beat. Therefore it must be decided if the beat grid should be shifted by a multiple of 1/16th of a beat. The first onset and the position of the kicks relative to the beat grid can give us clues. The assumptions is made that the first sound is on-beat and shift the beat grid to this, checking it makes sense looking at the distrubtion of kick offsets.

It is worth noting that there are several sanity checks on the properties of the extracted features, if any fail then the anaylsis fails and the track will not be used. This is better than 'clanging' the mix.

Once the beatgrid has been calculated some supplementary features can be extracted, the drops and the drums. A bandpass filter is used to measure the bass content of the track per 4 bars. Any 4 bars that have a bass content greater than the minimum plus 40% of the range are deemed to be in a 'drop'. The number of drums per 4 bars are also extracted.

An analysis log file per track will be output in the `$AUTOMIX_HOME/log` directory.

Mix
~~~

Tracks are mixed using an algorithm designed for DnB music, this may be suitable for other genres. The output mix will be fixed-tempo, this is set by the `-ot` argument (default 87.5 BPM)

First the number of breakdown sections required is calculated using the `-bd` argument (default 20). All tracks are examined to find tracks which have a section with few drums in the build-up before the first drop, these are deemed suitable breakdown sections to use. These sections will be evenly distributed throughout the mix.

For the other tracks either a 'normal' or 'double drop' transition will be performed. The `-dd` argument (default 20) sets the percentage of these transitions which should be double drops. For a double drop the track will drop 16 bars after the drop of the previous tune, with the mid and high frequencies of the previous track left playing for a further 16 bars. For a normal mix the track will drop at the end of the drop of the previous tune, the previous tune will not be played beyond this point.

Perform
~~~~~~~

The mix is performed by a virtual model of traditional DJ decks, there is a mixer with 6 channels. Actions can be performed on each of the channels. Tracks can be loaded, played and paused. And the track tempo, volume and LPF gain can be set.

The previous 'Mix' phase outputs a stack of such actions, each with a timestamp refering to the point in the output mix at which the action should happen. The 'decks' simply performs all the actions in the stack at the specified times until the last track finishes or all the channels are paused, compressing the output to MP3 and writing the output file.

Limitations
-----------

The primary limitation of Automix is the beatgrid extraction algorithm. The output of the QM Vamp BeatTrack plugin alone is not reliable enough, compared to proprietary solutions such as Serato, Rekordbox or Traktor which appear to extract the tempo to the nearest 0.05 BPM. As all further features and mixing points rely on the beatgrid, if it is wrong then the track will sound wrong when played in the mix. Some changes have been made to the plugin to increase the resolution, however it seems not to be the most suitable approach when the program has access to the whole track, which can be assumed to be constant tempo.

Further Work
------------

Some ideas for further work:

Analysis
~~~~~~~~

* Increase reliability of beat grid extraction algorithm, by using a method different to that currently used by the QM Vamp BeatTrack plugin. (Highest priority IMHO)
* Currently the kick extraction method is as likely to select a kick as a non-kick, improving this could increase the reliability of the beatgrid alignment.
* Detect Key
* Detect vocals
* Detect volume envelope
* More sophisticated drop detection

Mix
~~~

* In terms of the mixing algorithm the scope for further features is massive. Different algorithms for different genres and styles. Also different transition patterns and track selection algorithms based on extracted features.
* Volume trimming

Perform
~~~~~~~
* Bandpass filters and high and mid range EQ filters
* FX and looping support
