OnStep Telescope Controller
===========================
# Modification notes
Fork of release-4.24

Work in progress.

Configuration (eventually) intended for Keen-One derivative.

17HS15-1684S-PG27 specs:

Rated current: 1.68A

Gear ratio: 26.85:1

Step angle w/out gearbox: 1.80° (200 steps/rotation)

Harmonic drive (Luke_3D variant V5) specs:

Gear ratio: 79:1

Controller:

WeMos D1 R32 w/ CNC v3


Config for LV8729

Driver current max w/heatsink: 1.5A, default 0.8A

Rs: 0.1Ω

Vref: 0.675 (driver 90%)

Microstep resolution: 1/32 (for convenience)


Config for DRV8825

Driver current guideline w/heatsink: 1.5A

Rs: 0.1Ω

Vref: 0.84 (motor max), 0.756 (motor 90%), 0.75 (driver max), 0.675 (driver 90%)

Max microstep resolution: 1/32


===========================
# Important Note

THERE ARE SEVERAL GITHUB BRANCHES OF ONSTEP:
* The **RELEASE BRANCHES** are well tested and what most should use.  Usually the newest (highest revision) RELEASE is recommended.  No new features are added and only bug fixes where necessary and safe.
* Tne **BETA BRANCH**, if present, is a "snap-shot" of the MASTER where we have reached a point of apparent stability.  This provides access to most new features for adventurous users.
* The **MASTER BRANCH** is the most up to date OnStep version; where new features are added.  It is the least well tested branch and should only be user by experienced users willing to test for and report bugs.

# What is OnStep?
OnStep is a computerized telescope goto controller, based on Teensy or
Arduino control of stepper motors.

It supports Equatorial Mounts (GEM, Fork, etc.) as well as Alt-Az mounts
(including Dobsonians, and the like.)

OnStep was designed, from the beginning, as a more or less general purpose
system and provisions were made in the firmware to allow for use on a variety
of mounts.

# Features
OnStep supports a wide variety of connection options.  Either two or three serial
"command channels" can be utilized. One of the these is normally devoted to a USB
connection and for the other(s) choose from the following:

* Bluetooth
* ESP8266 WiFi
* Arduino M0/Ethernet Shield
* Even another USB port or RS232 serial isn't very difficult to add.

Other software in the OnStep ecosystem include:

* an [ASCOM](http://ascom-standards.org/) driver (with IP and Serial support),
* an Android App useable over WiFi or Bluetooth equipped Phones/Tablets
  (version 2.3.3 or later),
* a "built-in" website (on the Ethernet and/or WiFi device),
* a full planetarium program that controls all features ([Sky Planetarium](http://stellarjourney.com/index.php?r=site/software_sky)).

OnStep is compatible with the LX200 protocol. This means it can be controlled
from other planetarium software, like: Sky Safari, CdC (even without ASCOM),
Stellarium, etc.

There are also [INDI](http://www.indilib.org/about.html) drivers so it can be used from Linux, with CdC or KStars.

# Documentation
Detailed documentation, including the full set of features, detailed designs for
PCBs, instructions on how to build a controller, how to configure the firmware
for your particular mount, can all be found the [OnStep Group Wiki](https://groups.io/g/onstep/wiki/home).

# Change Log
All the changes are tracking in git, and a detailed list can be accessed using the
following git command:
 
git log --date=short --pretty=format:"%h %ad %<(20)%an %<(150,trunc)%s"

# Support
Questions and discussion should be on the mailing list (also accessible via the
web) at the [OnStep Group](https://groups.io/g/onstep/).

# License
OnStep is open source free software, licensed under the GPL.

See [LICENSE.txt](./LICENSE.txt) file.

# Author
[Howard Dutton](http://www.stellarjourney.com)
