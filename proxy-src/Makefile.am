SUBDIRS =	cmake src lib plugins examples scripts tests m4 doc

EXTRA_DIST = \
		config.h.cmake \
		autogen.sh \
		mysql-proxy.spec \
		winbuild.bat \
		CMakeLists.txt \
		mysql-chassis.pc.cmake \
		mysql-proxy.pc.cmake \
		deps \
		internal

ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mysql-proxy.pc mysql-chassis.pc

$(pkgconfig_DATA): config.status

dist-hook:
	rm -rf `find $(distdir) -type d -name .bzr -print`

## this check is in the toplevel Makefile intentionally,
## to catch the case when Lua is statically linked more than once 
## TODO: How do I wrestle the platform's shared library extension from automake's dead hands?
##       And why does libtool generate .so files on OS X?
installcheck-local:
	if test  `find $(DESTDIR)$(libdir) \( -name "*.so" -or -name "*.dylib" \) -and -type f | xargs nm -A -P | grep "_lua_gc T" | wc -l` -gt 1 ; then \
		echo "ERROR: multiple libraries export Lua library symbols. Do not statically link against Lua more than once!"; \
		echo "The following libraries contain Lua symbols:"; \
		find $(DESTDIR)$(libdir) -name "*.so" -or -name "*.dylib" -and -type f | xargs nm -A -P | grep "_lua_gc T" | cut -f 1 -d: ; \
		exit 1 ; \
	else:; fi
