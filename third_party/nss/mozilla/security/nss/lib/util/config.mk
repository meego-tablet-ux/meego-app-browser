#
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Netscape security libraries.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1994-2000
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

#
#  Override TARGETS variable so that only static libraries
#  are specifed as dependencies within rules.mk.
#

# can't do this in manifest.mn because OS_TARGET isn't defined there.
ifeq (,$(filter-out WIN%,$(OS_TARGET)))

# don't want the 32 in the shared library name
SHARED_LIBRARY = $(OBJDIR)/$(DLL_PREFIX)$(LIBRARY_NAME)$(LIBRARY_VERSION).$(DLL_SUFFIX)
IMPORT_LIBRARY = $(OBJDIR)/$(IMPORT_LIB_PREFIX)$(LIBRARY_NAME)$(LIBRARY_VERSION)$(IMPORT_LIB_SUFFIX)

RES = $(OBJDIR)/$(LIBRARY_NAME).res
RESNAME = $(LIBRARY_NAME).rc

ifdef NS_USE_GCC
EXTRA_SHARED_LIBS += \
	-L$(DIST)/lib \
	-L$(NSPR_LIB_DIR) \
	-lplc4 \
	-lplds4 \
	-lnspr4\
	$(NULL)
else # ! NS_USE_GCC
EXTRA_SHARED_LIBS += \
	$(NSPR_LIB_DIR)/$(NSPR31_LIB_PREFIX)plc4.lib \
	$(NSPR_LIB_DIR)/$(NSPR31_LIB_PREFIX)plds4.lib \
	$(NSPR_LIB_DIR)/$(NSPR31_LIB_PREFIX)nspr4.lib \
	$(NULL)
endif # NS_USE_GCC

else

# $(PROGRAM) has NO explicit dependencies on $(EXTRA_SHARED_LIBS)
# $(EXTRA_SHARED_LIBS) come before $(OS_LIBS), except on AIX.
EXTRA_SHARED_LIBS += \
	-L$(DIST)/lib \
	-L$(NSPR_LIB_DIR) \
	-lplc4 \
	-lplds4 \
	-lnspr4 \
	$(NULL)

endif

ifeq ($(OS_TARGET),SunOS)
ifeq ($(BUILD_SUN_PKG), 1)
# The -R '$ORIGIN' linker option instructs this library to search for its
# dependencies in the same directory where it resides.
ifeq ($(USE_64), 1)
MKSHLIB += -R '$$ORIGIN:/usr/lib/mps/secv1/64:/usr/lib/mps/64'
else
MKSHLIB += -R '$$ORIGIN:/usr/lib/mps/secv1:/usr/lib/mps'
endif
else
MKSHLIB += -R '$$ORIGIN'
endif
endif

ifeq ($(OS_ARCH), HP-UX) 
ifneq ($(OS_TEST), ia64)
# pa-risc
ifeq ($(USE_64), 1)
MKSHLIB += +b '$$ORIGIN'
endif
endif
endif
