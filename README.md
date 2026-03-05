My hardware implementation of USB MIDI host adapter based on this library: https://github.com/rppicomidi/EZ_USB_MIDI_HOST and this board: https://www.waveshare.com/wiki/RP2040-Zero

For host port I use onboard USB-C connector, no need for any modification. Use C-male to A-female (pictured) or C-female to B-male with UCB-C M-M cable.
5V power provided through additional USB-C dumb jack.
MIDI output driven by PNP follower, providing standard 5V MIDI current loop, that I prefer to 3.3V option to improve compartibility with older MIDI gear. My input made with 5V logic buffered opto and signal divided to match 3.3V input of PR2040. No specific reason for that opto, just what I was able to find in my stash.
