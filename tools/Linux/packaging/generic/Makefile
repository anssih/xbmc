-include ../../../depends/Makefile.include

DEP_LIBS = \
libFLAC.so \
libSDL.so \
libafpclient.so \
libass.so \
libbluray.so \
libcurl.so \
libmad.so \
libmpeg2.so \
libmpeg2convert.so \
libnfs.so \
libogg.so \
libplist.so \
librtmp.so \
libshairplay.so \
libsmbclient.so \
libvorbis.so \
libvorbisenc.so \
libvorbisfile.so \

SRCLIBS_BASE := $(addprefix $(PREFIX)/lib/,$(DEP_LIBS))
SRCLIBS_SONAME := $(addprefix $(PREFIX)/lib/,$(foreach lib,$(SRCLIBS_BASE),$(shell objdump -p $(lib) | grep SONAME | awk '{ print $$2 }')))
SRCLIBS_FILE := $(foreach lib,$(SRCLIBS_BASE),$(shell readlink -f $(lib)))

BUILD_DIR = $(CURDIR)/build
PKG_DIR = $(BUILD_DIR)/pkgbuild

ALLOWED_DEPS = $(BUILD_DIR)/allowed-deps.list

all: package-clean package

package-clean:
	rm -rf $(PKG_DIR)

package: install-deps install-xbmc install-wrapper postprocess-binaries
	tar -cj -C $(BUILD_DIR) --transform="s,^pkgbuild,xbmc-generix-linux-$(CPU)," -f $(BUILD_DIR)/xbmc-generix-linux-$(CPU).tar.bz2 pkgbuild
	@echo "$(BUILD_DIR)/xbmc-generix-linux-$(CPU).tar.bz2 created"

$(ALLOWED_DEPS): install-deps install-xbmc allowed-system-deps.list
	grep -v "^#" allowed-system-deps.list > $@
	for file in $$(find $(PKG_DIR) -type f); do \
		file $$file | grep -q "shared object" || continue; \
		$(CURDIR)/get-provs.sh $$file >> $@; \
	done

install-wrapper:
	mkdir -p $(PKG_DIR)
	install -m755 xbmc-wrapper.sh $(PKG_DIR)/xbmc

install-xbmc:
	mkdir -p $(PKG_DIR)/data/{bin,lib,share}
	cp -a $(PREFIX)/lib/xbmc $(PKG_DIR)/data/lib
	cp -a $(PREFIX)/share/xbmc $(PKG_DIR)/data/share
	cp -a $(PREFIX)/bin/xbmc* $(PKG_DIR)/data/bin
	sed -i "s,^prefix=.*$$,# set by caller #&," $(PKG_DIR)/data/bin/xbmc*
	find $(PKG_DIR)/data/share/xbmc/addons/skin.*/media -name '*.png' -delete

install-deps: $(SRCLIBS_SONAME) $(SRCLIBS_FILE)
	mkdir -p $(PKG_DIR)/data/{lib,bin}
	cp -dp $(SRCLIBS_SONAME) $(SRCLIBS_FILE) $(PKG_DIR)/data/lib
	cp -dpr $(PREFIX)/lib/python2.6 $(PKG_DIR)/data/lib

	# used by xbmc.sh, maybe get rid of this?
	cp -dp $(PREFIX)/bin/python2.6 $(PKG_DIR)/data/bin/python2.6
	ln -s python2.6 $(PKG_DIR)/data/bin/python

postprocess-binaries: install-deps install-xbmc $(ALLOWED_DEPS)
	fail=; for file in $$(find $(PKG_DIR) -type f); do \
		file $$file | grep -q "ELF" || continue; \
		deps="$$($(CURDIR)/get-deps.sh $$file | grep -Fxvf $(ALLOWED_DEPS))"; \
		if test -n "$$deps"; then \
			echo -e "Non-allowed deps in $$file:\n$$deps\n"; \
			fail=1; \
		else \
			echo "Deps OK: $$file"; \
		fi; \
		strip --strip-unneeded $$file || exit 1; \
	done; \
	[ -z "$$fail" ] || exit 1
