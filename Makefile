
# Copyright (C) 2001,2004-6,2012,2014,2016-7  Lancaster University
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# Contact Steven Simpson <https://github.com/simpsonst>

all::

CAT=cat
M4=m4
SED=sed
FIND=find
XARGS=xargs
ENABLE_CXX=yes
GETVERSION=git describe

PREFIX=/usr/local

VERSION:=$(shell $(GETVERSION) 2> /dev/null)

#CMHGFLAGS += -znoscl

# CFLAGS.shared = -fPIC
# CXXFLAGS.shared = -fPIC
# SHLINK.c = gcc -shared
# SHLINK.cc = g++ -shared -fPIC


## Provide a version of $(abspath) that can cope with spaces in the
## current directory.
myblank:=
myspace:=$(myblank) $(myblank)
MYCURDIR:=$(subst $(myspace),\$(myspace),$(CURDIR)/)
MYABSPATH=$(foreach f,$1,$(if $(patsubst /%,,$f),$(MYCURDIR)$f,$f))

-include $(call MYABSPATH,config.mk)
sinclude react-env.mk

test_binaries.c@riscos-rm += reactormodule
reactormodule_obj += module
reactormodule_obj += modmain
reactormodule_obj += atomics

riscos_zips += reactor
reactor_apps += system
system_appname=!System
system.reactormodule.modloc=Modules/310/ReactHelp

DOCS += HISTORY
DOCS += COPYING
DOCS += README
DOCS += VERSION

SOURCES:=$(filter-out $(headers),$(shell $(FIND) src/obj \( -name "*.c" -o -name "*.h" \) -printf '%P\n'))

reactor_apps += library
library_appname=!React
library_rof += !Boot,feb
library_rof += Docs/VERSION,fff
library_rof += Docs/COPYING,fff
library_rof += Docs/README,fff
library_rof += Source/cmhg/module,fff
library_rof += $(call riscos_hdr,$(headers))
library_rof += $(call riscos_src,$(SOURCES))
library_rof += $(call riscos_lib,$(libraries))


headers += $(REACT_HDRS:%=react/%)
REACT_HDRS += features.h
REACT_HDRS += types.h
REACT_HDRS += version.h
REACT_HDRS += prio.h
REACT_HDRS += core.h
REACT_HDRS += event.h
REACT_HDRS += conds.h
REACT_HDRS += condtype.h
REACT_HDRS += user.h
REACT_HDRS += idle.h
REACT_HDRS += socket.h
REACT_HDRS += time.h
REACT_HDRS += fd.h
REACT_HDRS += pipe.h
REACT_HDRS += file.h
REACT_HDRS += riscos.h
REACT_HDRS += windows.h
REACT_HDRS += signal.h

libraries += react

lc=$(subst A,a,$(subst B,b,$(subst C,c,$(subst D,d,$(subst E,e,$(subst F,f,$(subst G,g,$(subst H,h,$(subst I,i,$(subst J,j,$(subst K,k,$(subst L,l,$(subst M,m,$(subst N,n,$(subst O,o,$(subst P,p,$(subst Q,q,$(subst R,r,$(subst S,s,$(subst T,t,$(subst U,u,$(subst V,v,$(subst W,w,$(subst X,x,$(subst Y,y,$(subst Z,z,$1))))))))))))))))))))))))))

ifneq ($(filter true t y yes on 1,$(call lc,$(ENABLE_CXX))),)
REACT_HDRS += condition.hh
REACT_HDRS += handler.hh
REACT_HDRS += prio.hh
REACT_HDRS += version.hh
REACT_HDRS += core.hh
REACT_HDRS += event.hh
REACT_HDRS += condhelp.hh
REACT_HDRS += conds.hh
REACT_HDRS += handlerhelp.hh
REACT_HDRS += user.hh
REACT_HDRS += idle.hh
REACT_HDRS += time.hh
REACT_HDRS += windows.hh
REACT_HDRS += socket.hh
REACT_HDRS += fd.hh
REACT_HDRS += types.hh
REACT_HDRS += file.hh
REACT_HDRS += pipe.hh

libraries += react++
endif


react++_mod += cltests
react++_mod += cppevent
react++_mod += cppcore

react_mod += time
react_mod += array
react_mod += portable
react_mod += core
react_mod += prime
react_mod += signals
react_mod += winhandle
react_mod += primetime
react_mod += fdextract
react_mod += select
react_mod += poll
react_mod += fd
react_mod += file
react_mod += pipe
react_mod += idle
react_mod += version
react_mod += yield
react_mod += socket
react_mod += riscos

test_binaries.c += speedtest
speedtest_obj += speed
speedtest_obj += $(react_mod)
speedtest_lib += -lddslib
speedtest_lib += $(THREADLIBS)
speedtest_lib += $(SOCKLIBS)

test_binaries.c += echod
echod_obj += echod
echod_obj += $(react_mod)
echod_lib += -lddslib
echod_lib += $(SOCKLIBS)
echod_lib += $(THREADLIBS)

test_binaries.c += testreact
testreact_obj += testreact
testreact_obj += $(react_mod)
testreact_lib += -lddslib
testreact_lib += $(SOCKLIBS)
testreact_lib += $(THREADLIBS)

test_binaries.c += proxy
proxy_obj += proxy
proxy_obj += $(react_mod)
proxy_lib += -lddslib
proxy_lib += $(SOCKLIBS)
proxy_lib += $(THREADLIBS)

test_binaries.cc += cppproxy
cppproxy_obj += cppproxy
cppproxy_obj += $(react++_mod)
cppproxy_obj += $(react_mod)
cppproxy_lib += -lddslib
cppproxy_lib += $(SOCKLIBS)
cppproxy_lib += $(THREADLIBS)

test_binaries.cc += cpptest
cpptest_obj += cpptest
cpptest_obj += $(react++_mod)
cpptest_obj += $(react_mod)
cpptest_lib += -lddslib
cpptest_lib += $(SOCKLIBS)
cpptest_lib += $(THREADLIBS)

include binodeps.mk

all:: installed-libraries VERSION

install:: install-libraries install-headers

ifneq ($(filter true t y yes on 1,$(call lc,$(ENABLE_RISCOS))),)
all:: riscos-apps riscos-zips
tmp/obj/yield.o tmp/obj/core.o: tmp/obj/module.h
endif

tmp/obj/version.o: tmp/obj/react/version.h
tmp/obj/modmain.mo: tmp/obj/module.h

$(BINODEPS_OUTDIR)/riscos/!React/Docs/README,fff: README.md
	@$(PRINTF) '[Copy RISC OS export] %s\n' $<
	@$(MKDIR) "$(@D)"
	@$(CP) "$<" "$@"

$(BINODEPS_OUTDIR)/riscos/!React/Docs/COPYING,fff: LICENSE.txt
	@$(PRINTF) '[Copy RISC OS export] %s\n' $<
	@$(MKDIR) "$(@D)"
	@$(CP) "$<" "$@"

$(BINODEPS_OUTDIR)/riscos/!React/Docs/VERSION,fff: VERSION
	@$(PRINTF) '[Copy RISC OS export] %s\n' $<
	@$(MKDIR) "$(@D)"
	@$(CP) "$<" "$@"

ifneq ($(VERSION),)
prepare-version::
	@$(MKDIR) tmp/
	@$(ECHO) $(VERSION) > tmp/VERSION

tmp/VERSION: | prepare-version
VERSION: tmp/VERSION
	@$(CMP) -s '$<' '$@' || $(CP) '$<' '$@'
endif

tmp/obj/react/version.h: src/obj/react/version.h.m4 VERSION
	@$(PRINTF) '[version header %s]\n' "$(file <VERSION)"
	@$(MKDIR) '$(@D)'
	@$(M4) -DVERSION='`$(file <VERSION)'"'" < '$<' > '$@'

tidy::
	-$(FIND) . -name "*~" -delete

# Set this to the comma-separated list of years that should appear in
# the licence.  Do not use characters other than [0-9,] - no spaces.
YEARS=2001,2004-6,2012,2014,2016-7

update-licence:
	$(FIND) . -name ".svn" -prune -or -type f -print0 | $(XARGS) -0 \
	$(SED) -i 's/Copyright (C) [0-9,-]\+  Lancaster University/Copyright (C) $(YEARS)  Lancaster University/g'
