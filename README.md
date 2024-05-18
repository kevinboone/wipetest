# wipetest

Version 0.1a

A simple utility for Linux, for preparing used hard drives for sale

## What is this?

`wiptetest` is a utility for preparing used hard drives for sale, 
when unauthorized recovery of the data represents only a modest security risk.
It overwrites every block of data with a meaningless pattern, and
checks that every block can be read back correctly. It does these things
only once per disk block, and the data pattern 
is not random. As a result, this is not the utility
to use if your disk contains state secrets or your plans for world
domination. 

`wipetest` attempts to be _reasonably_ sure that the overwritten
data is _reasonably_ hard to recover, and that it is _reasonably_
likely that the disk is usable.  

`wipetest` focuses on speed and convenience. Utilities like 
`wipe` are more appropriate for high-security scenarios. The process
used by `wipetest` will defeat casual attempts to recover the data, 
but won't stand up to the kind of low-level analysis that security
agencies have at their disposal. 

## Usage

    $ wipetest [options] {block_device}

For example

    $ wipetest /dev/sdc

Usually `wipetest` will check if a drive is mounted as a filesystem,
and ask the user to confirm that the data should be destroyed.
The `-f/--force` option skips these checks.

`wiptest` writes limited progress information as it works.

## Limitations

Writing and reading every sector of a large disk is slow, even 
without the contortions that a more secure data eraser would go through.
Expect about 500Gb per hour for a USB 3 drive.

## Caution!

`wipetest` is a utility for destroying data. It can destroy wanted
data just as rapidly as unwanted data. It does relatively few checks:
the onus is on the user to use is correctly.

## Author and legal

`wipetest` is maintained by Kevin Boone and distributed under the 
terms of the GNU Public Licence, v3.0. There is no warranty of any 
kind.



