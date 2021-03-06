#################################################################
#								#
# Copyright (c) 2013-2015 Fidelity National Information 	#
# Services, Inc. and/or its subsidiaries. All rights reserved.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

# Default configuration parameters. These values can be overridden by passing different values to build different build targets. For
# the list of available build targets, see below.
image = DEBUG
thirdparty = gcrypt
algo = AES256CFB
# If the machine has a libgcrypt version <= 1.4.1, then FIPS mode cannot be turned on.
gcrypt_nofips = 0
# Default HP-UX OpenSSL include/lib base directory.
HPUX_OPENSSL_ROOT = /opt/openssl/1.0.1e/

# Verify that $gtm_dist is defined
ifndef gtm_dist
$(error $$gtm_dist not defined!)
endif

DISTDIR = $(gtm_dist)
PLUGINDIR = $(DISTDIR)/plugin
GTMCRYPTDIR = $(PLUGINDIR)/gtmcrypt
CURDIR = `pwd`

# Find out whether we are already in $gtm_dist/plugin/gtmcrypt directory.
NOT_IN_GTMCRYPTDIR = $(shell [ "$(CURDIR)" = "$(GTMCRYPTDIR)" ] ; echo $$?)

# Determine machine and OS type.
UNAMESTR = $(shell uname -a)
MACHTYPE = $(shell uname -m)

ifneq (,$(findstring Linux,$(UNAMESTR)))
	FILEFLAG = -L
endif
# 64 bit version of GT.M? 0 for yes!
BIT64 = $(shell file $(FILEFLAG) $(DISTDIR)/mumps | grep -q -E '64-bit|ELF-64'; echo $$?)

# Default installation target. This allows for the build system to randomize `thirdparty' and `algo' thereby changing the default
# gtmcrypt install link.
install_targ = libgtmcrypt_$(thirdparty)_$(algo).so

# Setup build type -- debug or production.
ifneq ($(image),DEBUG)
	debug_flag =
	optimize = -O2
else
	debug_flag = -g -DDEBUG
	optimize =
endif

CC = cc

# Setup common compiler flags
CFLAGS = $(debug_flag) $(optimize) -D_FILE_OFFSET_BITS=64 -D_LARGE_FILES -D_LARGEFILE64_SOURCE=1

ifneq ($(gcrypt_nofips),0)
	gcrypt_nofips_flag = -DGCRYPT_NO_FIPS
else
	gcrypt_nofips_flag =
endif

# Set default extra CFLAGS and LDFLAGS for building maskpass and libgtmcryptutil.so. Both of these use SHA512 for password
# obfuscation which must come from either OpenSSL or Libgcrypt. Historically, on AIX we have always made OpenSSL the default
# library and on non-AIX platforms, it has been libgcrypt.
default_thirdparty_CFLAGS = -DUSE_GCRYPT
default_thirdparty_LDFLAGS = -lgcrypt -lgpg-error

# Platform specific compiler and linker flags
# Linux
LIBFLAGS =
IFLAGS =
ifneq (,$(findstring Linux,$(UNAMESTR)))
	# -fPIC for Position Independent Code.
	CFLAGS += -fPIC
	LDFLAGS =
	# So that dependent libraries are loaded from the parent library's load path at runtime
	RPATHFLAGS = -Wl,-rpath,'$$ORIGIN'
	# So that we can build shared library.
	LDSHR = -shared
endif

# Solaris
ifneq (,$(findstring Solaris,$(UNAMESTR)))
	# -fPIC for Position Independent Code; -m64 for 64-bit
	CFLAGS += -fPIC -m64
	LDFLAGS = -Wl,-64 -m64
	# So that dependent libraries are loaded from the parent library's load path at runtime
	RPATHFLAGS = -Wl,-R,'$$ORIGIN'
	LDSHR = -G
endif

# HP-UX
ifneq (,$(findstring HP-UX,$(UNAMESTR)))
	# +Z is for Position Independent Code; -Ae for Extended ANSI mode and +DD64 for 64-bit
	CFLAGS += +Z -Ae  +DD64
	LDFLAGS = +DD64
	# So that dependent libraries are loaded from the parent library's load path at runtime
	RPATHFLAGS = -Wl,+b,\$$ORIGIN
	# -b for shared library and -B,symbolic for assigning protected export calls to symbols.
	LDSHR = -Wl,-b,-B,symbolic
	LIBFLAGS = -L $(HPUX_OPENSSL_ROOT)/lib
	IFLAGS = -I $(HPUX_OPENSSL_ROOT)/include
endif

# AIX
ifneq (,$(findstring AIX,$(UNAMESTR)))
	# -qchars=signed forces `char' type to be treated as signed chars.
	# -qsrcmsg enhances error reporting.
	# -qmaxmem limits the amount of memory used for local tables of specific, memory-intensive operations (in kilobytes).
	CFLAGS += -qchars=signed -qsrcmsg -qmaxmem=8192 -D_TPARM_COMPAT

	# -qro places string literals in read-only storage.
	# -qroconst places constants in read-only storage.
	# -q64 forces 64-bit object generation
	CFLAGS += -qro -qroconst -q64
	# -q64 for 64-bit object generation
	# -brtl for allowing both '.a' and '.so' to be searched at runtime.
	# -bhalt:5 is to disable warnings about duplicate symbols that come from
	#  libgtmcryptutil.so and other .so that need pulling the same object
	#  file (/lib/crt0_64.o)
	LDFLAGS = -q64 -brtl -bhalt:5
	RPATHFLAGS =
	# -G so that we can build shared library
	# -bexpall exports all symbols from the shared library.
	# -bnoentry to tell the linker that shared library has no entry point.
	LDSHR = -Wl,-G -bexpall -bnoentry
	# On AIX, build maskpass and libgtmcryptutil.so with OpenSSL's libcrypto instead of libgcrypt.
	default_thirdparty_CFLAGS = -DUSE_OPENSSL
	default_thirdparty_LDFLAGS = -lcrypto
endif

# Common header and library paths
IFLAGS += -I /usr/local/ssl/include -I /usr/local/include -I /usr/include -I $(gtm_dist) -I $(CURDIR)
ifeq ($(BIT64),0)
	LIBFLAGS += -L /usr/local/ssl/lib -L /usr/lib/x86_64-linux-gnu -L /usr/local/lib64
	LIBFLAGS += -L /usr/local/lib -L /usr/lib64 -L /usr/lib -L /lib64 -L /lib -L `pwd`
else
	LIBFLAGS += -L /usr/local/ssl/lib -L /usr/lib/x86-linux-gnu -L /usr/local/lib32
	LIBFLAGS += -L /usr/local/lib -L /usr/lib32 -L /usr/lib -L /lib32 -L /lib -L `pwd`
endif

CFLAGS += $(IFLAGS)
LDFLAGS += $(LIBFLAGS) -o

COMMON_LIBS = -lgtmcryptutil -lconfig

# Lists of all files needed for building the encryption plugin.
crypt_util_srcfiles = gtmcrypt_util.c
crypt_util_hdrfiles = gtmcrypt_util.h gtmcrypt_interface.h
crypt_srcfiles = gtmcrypt_ref.c gtmcrypt_pk_ref.c gtmcrypt_dbk_ref.c gtmcrypt_sym_ref.c
crypt_hrdfiles = gtmcrypt_ref.h gtmcrypt_pk_ref.h gtmcrypt_dbk_ref.h gtmcrypt_sym_ref.h gtmcrypt_interface.h
tls_srcfiles = gtm_tls_impl.c
tls_hdrfiles = gtm_tls_impl.h gtm_tls_interface.h

all: libgtmcryptutil.so maskpass gcrypt openssl libgtmtls.so

libgtmcryptutil.so: $(crypt_util_srcfiles) $(crypt_util_hdrfiles)
	@echo ; echo "Compiling $@..."
	$(CC) $(CFLAGS) $(default_thirdparty_CFLAGS) $(crypt_util_srcfiles) $(LDSHR) $(LDFLAGS) $@ $(default_thirdparty_LDFLAGS)

# Since maskpass is a standalone utility, link it (implicitly) with gtmcrypt_util.o instead of libgtmcryptutil.so. This allows
# maskpass to be run without setting LD_LIBRARY_PATH/LIBPATH to load libgtmcryptutil.so. As a standalone utility maskpass should
# not depend on functions like `gtm_malloc' and `gtm_free', so we are compiling the executable with -DUSE_SYSLIB_FUNCS.
maskpass: maskpass.c $(crypt_util_srcfiles) $(crypt_util_hdrfiles)
	@echo ; echo "Compiling $@..."
	$(CC) $(CFLAGS) -DUSE_SYSLIB_FUNCS $(default_thirdparty_CFLAGS) maskpass.c $(crypt_util_srcfiles)	\
		$(LDFLAGS) $@ $(default_thirdparty_LDFLAGS)

gcrypt: libgtmcrypt_gcrypt_AES256CFB.so

libgtmcrypt_gcrypt_AES256CFB.so: $(crypt_srcfiles) $(crypt_hdrfiles) libgtmcryptutil.so
	@echo ; echo "Compiling $@..."
	$(CC) $(CFLAGS) -DUSE_GCRYPT -DUSE_AES256CFB $(gcrypt_nofips_flag) $(crypt_srcfiles) $(LDSHR)		\
		$(RPATHFLAGS) $(LDFLAGS) $@ -lgcrypt -lgpgme -lgpg-error $(COMMON_LIBS)

openssl: libgtmcrypt_openssl_AES256CFB.so libgtmcrypt_openssl_BLOWFISHCFB.so

libgtmcrypt_openssl_AES256CFB.so: $(crypt_srcfiles) $(crypt_hdrfiles) libgtmcryptutil.so
	@echo ; echo "Compiling $@..."
	$(CC) $(CFLAGS) -DUSE_OPENSSL -DUSE_AES256CFB $(crypt_srcfiles) $(LDSHR) $(RPATHFLAGS) $(LDFLAGS)	\
		$@ -lcrypto -lgpgme -lgpg-error $(COMMON_LIBS)

libgtmcrypt_openssl_BLOWFISHCFB.so: $(crypt_srcfiles) $(crypt_hdrfiles) libgtmcryptutil.so
	@echo ; echo "Compiling $@..."
	$(CC) $(CFLAGS) -DUSE_OPENSSL -DUSE_BLOWFISHCFB $(crypt_srcfiles) $(LDSHR) $(RPATHFLAGS) $(LDFLAGS)	\
	$@ -lcrypto -lgpgme -lgpg-error $(COMMON_LIBS)

libgtmtls.so: $(tls_srcfiles) $(tls_hdrfiles) libgtmcryptutil.so
	@echo ; echo "Compiling $@..."
	$(CC) $(CFLAGS) $(tls_srcfiles) $(LDSHR) $(RPATHFLAGS) $(LDFLAGS) $@ -lssl $(COMMON_LIBS)

install: all
	@echo ; echo "Installing shared libraries to $(PLUGINDIR) and maskpass to $(PLUGINDIR)/gtmcrypt..."
	cp -f *.so $(PLUGINDIR)
	ln -fs ./$(install_targ) $(PLUGINDIR)/libgtmcrypt.so
ifeq ($(NOT_IN_GTMCRYPTDIR),1)
	cp -f maskpass $(PLUGINDIR)/gtmcrypt
endif

uninstall:
	@echo ; echo "Uninstalling shared libraries from $(PLUGINDIR) and maskpass from $(PLUGINDIR)/gtmcrypt..."
	rm -f $(PLUGINDIR)/*.so
	rm -f $(PLUGINDIR)/gtmcrypt/maskpass

clean:
	@echo ; echo "Removing generated files..."
	rm -f *.so *.o
ifeq ($(NOT_IN_GTMCRYPTDIR),1)
	rm -f maskpass
endif
