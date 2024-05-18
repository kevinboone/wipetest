# wipetest

Version 0.1b

A simple utility for Linux, for preparing used hard drives for sale

## What is this?

`wiptetest` is a utility for preparing used hard drives for sale, 
in circumstances where unauthorized recovery of their data represents no more
than a modest security hazard. It's designed for speed and convenience,
rather than military-grade security. 

## Why do we need this?

If you sell a used hard drive, it's likely to be to a stranger. Used drives
don't fetch high prices, and there are people out there who are buying them
in the hope of finding sensitive data on them.

Merely reformatting a drive will do _nothing_ to prevent this kind of attack.
Nearly all the original data will still be in place on the drive, and there are
well-known methods for recovering most of it. Worse, software exists to scan
disks for interesting data patterns that might indicate the 
presence of useful (to the
intruder) data.

Using whole-disk encryption, or even file-by-file encryption, can
lessen these risks considerably. However, even this approach 
isn't indefeasible 
-- people have been coerced or tricked into handing over their passwords.

What's needed is a method to ensure that the data is really gone, forever.
This is probably impossible if the intruder is a national
security agency; but it's possible to defeat the efforts of 
most villains without a huge amount of effort.

There is a balance to be found here, between the time it takes to 
erase and test a drive, and the strength of the attack you want to 
defeat. Utilities are available
which will erase data in such a way that only major military organizations
stand any chance of recovering it. However, these utilities are generally
very, very slow. A simple block-by-block overwrite of the entire
disk, ideally with randomized data, will make the data unrecoverable
for all practicable purposes. Of course, if your disk contains state
secrets or billions or credit-card details, 'all practicable purposes'
might not be enough.

Still, in that case, it makes no sense to be selling used drives on the
second-hand market. They don't fetch high prices, after all. It would
be better to put the drive in a fire, and then smash it up with a 
club-hammer. 

If you do sell a hard drive -- or a computer containing a hard drive --
it's advisable to ensure that the drive actually works. One fairly thorough
way to do that is to read back the data you just wrote as you wiped
the disk, block by
block. Again, there are more robust methods, but if you can write and
read every sector, one by one, you can probably sell the drive as
'working' with a clean conscience.

In short,
`wipetest` attempts to allow you to be _reasonably_ sure that any data
that was on a storage drive 
is _reasonably_ hard to recover, and that it is _reasonably_
likely that the disk is usable enough to sell.  

## What `wipetest` does

`wipetest` overwrites each block of data with a meaningless pattern and, when
it's done that, it reads every block in the same order it wrote them, and
verifies that the data it reads matches what it wrote. 

There are better tools if you really, really can't risk
your data being recovered -- including, of course, the bonfire and
club-hammer.

## Caution!

_`wipetest` is a utility for destroying data_. It can destroy wanted
data just as readily as unwanted data. It does relatively few checks:
the onus is on the user to operate it correctly.

## Usage

    $ wipetest [options] {block_device}

For example

    $ wipetest /dev/sdc

`block_device` will usually be a whole-disk device, but it's possible
to wipe a specific partition instead, if the device supports partitions
(most do).

`wipetest` writes a limited amount of progress information as it works.

With no command-line options, `wipetest` writes a fixed data pattern
repeatedly to the disk. If you're more paranoid, you can use the
`--random` switch, to write a stream of pseudo-random digits instead.

If you only want to write the data, and not test that it reads back
correctly, use the `--wipeonly` switch.

### -f, --force            

Usually `wipetest` will check if a drive is mounted as a filesystem,
and ask the user to confirm that the data should be destroyed.
The `-f/--force` option skips these checks.

### -r, --random           

Use a randomized data pattern, rather fixed data. A little slower, but
only noticeable so with very fast drives.

### -v, --version         

Show the version.

### -w, --wipeonly       

Only wipe the disk -- don't check that the data can be read back.

## Limitations

Writing and reading every sector of a large disk is slow, even 
without the contortions that a more secure data eraser would go through.
Expect about 500Gb per hour for a USB 3 drive.

The randomization of the data written to the disk (with the `-r` 
switch) is not cryptographically strong -- it just uses the random-number
generator in the C standard library, seeded with the time of day. 

`wipetest` doesn't check, or report, what kind of filesystem a drive contains.
So you can't use this information to help you decide whether you're 
about the wipe the right device. 

## Technical notes

### Synchronization and buffering

`wipetest` opens the disk device in the `O_DIRECT` mode, which means that
it requests the operating system not to buffer any data. However, there are 
many levels of buffer between the application and the disk hardware 
-- including the butters in the drive itself, which can't readily be
controlled. This is potentially important because, if the computer
crashes or shuts down during the wipe, at least some of the sectors will remain
unwiped.  

But, frankly, if this happens, that's an indication to repeat the whole
process. The more pragmatic reason to use `O_DIRECT` is that `wipetest`
can provide a plausible indication of progress. On a modern Linux system,
disk buffers are likely to be enormous. If the system is allowed to 
buffer freely, it might appear to write tens of thousands of blocks in
seconds, and then do nothing for minutes on end, as the buffers are cleared.

`wipetest` calls `sync()` only once, just before it exits. At the command
line, when the program returns to the prompt, you can be reasonably sure
that all changes have been committed -- to the drive controller, if not
yet to the drive substrate itself. 

### Randomization

It makes sense to write randomized data to the drive, if practicable.
Not only does this make unauthorized data recovery a little harder, 
it means that the read-back test is really testing the storage, not
just the disk cache. With a fixed data pattern, the disk read operation
could fail, and the operating system just return the data from a previous
read that is in the cache. If there is a fixed data pattern, that could
lead to a false positive read test. With a random pattern, that's 
unlikely. To be fair, it's pretty unlikely anyway.

The randomization process is not cryptographically secure -- it's intended to
be reasonably fast. Whether writing random data patterns is significantly
slower than fixed patterns (it's always at least a little slower) depends on
the speed of the drive. On a relative slow drive (a USB memory stick, for
example), randomization will not make the process significantly slower.

### Repeats

`wipetest` only reads and writes each sector a single time. That's already 
slow enough on a large drive, without multiplying the time by repeating
the process multiple times. However, if a 
single wipe isn't sufficient, for some reason, you could just run the
utility again. Because of the way that random number generator seeded,
it's exceedingly unlikely that `wipetest` will write the same data on successive
runs.

If you do run it multiple times, it's probably helpful to use the
`--wipeonly` switch after the first time. 

### Bad sectors

`wipetest` stops immediately if it can't read or write data, or if the data
it reads does not match the data it wrote. It doesn't build up a list of
bad sectors, to be stored in a filesystem, for example. 

In practice, good-quality, modern storage devices don't usually
report bad sectors unless they are very badly broken. This is because
these drives maintain an internal list of bad sectors, and have a surplus
of sectors to replace them with. 

This design increases the long-term reliability of the drive, which is generally
welcome. Unfortunately, it means that private data can get trapped
on the drive, in a way that no general-purpose application can get at it.
Vendor-specific diagnostic software might still be able to see this data. 

As ever, there's a cost/risk balance here. If you think that somebody
might want your data badly enough to recover a few sectors of it using
vendor-specific tooling, it's time for the bonfire and club-hammer
again.

### Removable media

`wipetest` will work on USB sticks and the like, but it will take a long
time. These devices usually offer very poor sustained write speeds, because
their usual application doesn't require them to. 

## Author and legal

`wipetest` is maintained by Kevin Boone and distributed under the 
terms of the GNU Public Licence, v3.0. There is no warranty of any 
kind.

# Revision history

0.1b May 2024
Added randomization feature
Added 'wipe only' feature


