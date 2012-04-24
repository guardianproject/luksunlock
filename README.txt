Minimal UI for LUKS encryption on the Wildfire

  This is a basic gui I wrote to unlock my encrypted partitions during boot.
  I'm running my /data and /sdcard partitions encrypted, and the "luksunlock"
  binary is launched from init.rc to read the password and unlock the
  encrypted partitions.

  I have included my somewhat modified init.rc for those interested.  For more
  information about LUKS on Android see this blogpost, written by shawn (Seems
  I'm not allowed to have urls in the post, but Google for 'android luks' ,
  first hit)

  This works good on Wildfire, altough it should work fine on other phones as
  well. Just remember that you need to set up your partitions as in the
  luksunlock.c (or change the defines).

  Don't forget to backup before you start playing around!
  Good luck!

 from http://forum.xda-developers.com/showthread.php?t=866131
 posted by sigkill1337

here's the direct link to the tarball:
http://forum.xda-developers.com/attachment.php?attachmentid=459501&d=1291847781

And the init.rc.txt:
http://forum.xda-developers.com/attachment.php?attachmentid=459500&d=1291847781


--------
Building
--------

You'll need some .so files from either an Android emulator or your phone, just
start the emulator or plug in your phone to USB in dev mode, so 'adb pull' can
get the files.

Here's the current general idea for building:

 cd luksunlock
 git submodule init
 git submodule update
 make -C external
 make -C external libpng-build
 make -C external libpng-install
 make
