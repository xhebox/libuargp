# libuargp

extracted from (uclibc-ng)[https://github.com/wbx-github/uclibc-ng].

what happened to musl's intl & gnu's argp:

+ https://www.openwall.com/lists/musl/2017/03/24/10

+ http://lists.gnu.org/archive/html/bug-gnulib/2019-01/msg00046.html

+ https://github.com/coreutils/gnulib/commit/ff3fe55c7c95bdb9460a40747912b4b309519838#diff-e8acc63b1e238f3255c900eed37254b8

+ https://github.com/coreutils/gnulib/commit/5f985580b9473b951eff13d92db613d464884091#diff-e8acc63b1e238f3255c900eed37254b8

**currently, gnu has fixed this bug.**

so this project is to provide another choice than the old argp-standalone.

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
