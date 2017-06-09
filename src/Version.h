#ifndef VERSION_H
#define VERSION_H

#include <string>

class Version {
public:
	// Defined by build system:
	static const bool RELEASE;
	static const char* const VERSION;
	static const char* const REVISION;

	// Computed using constants above:
	static std::string full();
};

#endif // VERSION_H
