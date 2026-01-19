CC=gcc
CFLAGS=-std=c11 -Wall -Wextra -O2 -D_POSIX_C_SOURCE=200112L

all: smbbroker smbpublish smbsubscribe

smbbroker: smbbroker.c smb.h
	$(CC) $(CFLAGS) -o smbbroker smbbroker.c

smbpublish: smbpublish.c smb.h
	$(CC) $(CFLAGS) -o smbpublish smbpublish.c

smbsubscribe: smbsubscribe.c smb.h
	$(CC) $(CFLAGS) -o smbsubscribe smbsubscribe.c

clean:
	rm -f smbbroker smbpublish smbsubscribe