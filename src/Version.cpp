// $Id$

#include "Version.h"

#include "Version.ii"

const std::string Version::FULL_VERSION
	= "openMSX Debugger " + Version::VERSION
	+ ( Version::RELEASE ? "" : "-dev" + Version::CHANGELOG_REVISION );
