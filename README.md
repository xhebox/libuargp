# libuargp

extracted from (uclibc-ng)[https://github.com/wbx-github/uclibc-ng]. it's to be compatible with musl's intl.

what happened to musl's intl:

+ https://www.openwall.com/lists/musl/2017/03/24/10

+ https://www.gnu.org/software/gettext/manual/html_node/Interface-to-gettext.html

# usage

remove all files, and link the modified version:

```
truncate -s 0 gnulib/lib/argp*
ln -sf /usr/include/argp.h gnulib/lib/argp.h
LDFLAGS="$LDFLAGS -l:libargp.so.0"
```

or just overwrite, since argp symbols from gnulib are weak symbols:

```
CFLAGS="$CFLAGS -l:libargp.so.0"
```
