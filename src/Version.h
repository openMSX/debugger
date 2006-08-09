// $Id$

#ifndef VERSION_H
#define VERSION_H

#include <string>

class Version {
public:
	// Defined by build system:
	static const bool RELEASE;
	static const std::string VERSION;
	static const std::string CHANGELOG_REVISION;

	// Computed using constants above:
	static const std::string FULL_VERSION;
};

#endif // VERSION_H
