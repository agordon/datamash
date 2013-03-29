# Copyright (C) 2013 Assaf Gordon <assafgordon@gmail.com>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.


##
## This script will be installed to the "bin" directory,
## together with the main compiled program.
## will be available to the user on the $PATH .
##
bin_SCRIPTS = example.sh


##
## This script will be installed in the "/usr/share/APPNAME/scripts" directory.
## Available to users after installation, but not on the "$PATH".
## Suitible for support scripts that's not commonly used.
##
scriptdir = $(pkgdatadir)/scripts

script_SCRIPTS = support.sh

EXTRA_DIST = $(script_SCRIPTS) \
	$(bin_SCRIPTS)

