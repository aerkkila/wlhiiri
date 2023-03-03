This program is similar to the Mouseless add-on for Firefox.
This program is run when user wants to click something with mouse.
This program puts texts on the screen and user types that text which is in the right place to click.

To be able to see the screen behind the texts, the color of fullscreen background on wayland compositor must have zero alpha channel.
For example on dwl: \
static const float fullscreen\_bg[]         = {0.0, 0.0, 0.0, 0.0};

*Installation:*
  make \
  make install # as root

The main program does not create the texts that go to screen.
The texts are created by perl-script luo\_sanat.pl.
Its usage is ./luo\_sanat.pl file\_in > sanat
That reads the file\_in and parses it into usable format and prints to stdout which should be redirected to file called sanat.
Makefile runs that program using LICENSE as file\_in.

The program is installed with SUID permission because opening /dev/uinput with write access to create a mouse needs root privilidges.
However, I don consider that as a security issue, since the program gives up root privilidges right after opening that file before doing anything else (see the main function).

Usage: \
type the text \
+ \
Return to click on the spot of the text. \
Arrow Up / Arrow Down to click on the spot of the text and then move cursor to right border of the screen. \
Esc to exit.
