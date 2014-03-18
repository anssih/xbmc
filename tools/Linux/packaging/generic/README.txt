#########################
XBMC Generic Linux builds
#########################

Development headers for X11 and Mesa are needed.

1. in tools/depends:
 $ ./bootstrap
 $ CFLAGS="-O2 -pipe" CXXFLAGS="-O2 -pipe" ./configure --with-toolchain=/usr --prefix=/opt/xbmc-deps
 $ make (-j20)
 $ make -C target/xbmc
2. in xbmc root:
 $ make (-j20)
 $ make install
3. in xbmc root:
 $ make -C tools/Linux/packaging/generic

To make a 32-bit build on a 64-bit system, you can use the 32-bit
personality ("linux32" command) for build.

################
TODO:
################
- Eliminate unwanted libraries from allowed-system-deps.list either
  a) by adding them to depends, or
  b) by using build options to not use them.
- Check if there are any static-from-depends libraries which should be
  replaced with system versions.
- fix 32-bit build
  * something wrong with native python-distribute
  * some additions to allowed-system-deps.list needed based on build result
- Python handled OK?
- fix everything that does not work
- create separate debug symbol package (or simply an alternate package
  with debug symbols) instead of simply stripping symbols
- harfbuzz support in libass?
- idn support in curl?
- livudev?
- check licensing
- name of the final tarball
- this README.txt
