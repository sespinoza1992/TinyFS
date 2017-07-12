tinyFsDemo: tinyFsDemo.c libDisk.c libDisk.h libTinyFS.c libTinyFS.h TinyFS.c TinyFS.h TinyFS_errno.h
	gcc -o tinyFsDemo tinyFsDemo.c libDisk.c libTinyFS.c TinyFS.c

debug: driver.c libDisk.c libDisk.h libTinyFS.c libTinyFS.h TinyFS.c TinyFS.h TinyFS_errno.h
	gcc -Wall -o debugtfs driver.c libDisk.c libTinyFS.c TinyFS.c

compress: tinyFsDemo.c libDisk.c libDisk.h libTinyFS.c libTinyFS.h TinyFS.c TinyFS.h TinyFS_errno.h
	tar -zcvf TinyFS.tgz tinyFsDemo.c libDisk.c libDisk.h libTinyFS.c libTinyFS.h TinyFS.c TinyFS.h TinyFS_errno.h makefile README

clean:
	rm -fv debugtfs tinyFsDemo disk*.dsk disk*.disk tinyFSDisk
