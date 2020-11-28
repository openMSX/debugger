#ifndef OPENMSX_H
#define OPENMSX_H

// don't just always include this, saves about 1 minute build time!!
#ifdef DEBUG
#include <iostream>
#endif

/// Namespace of the openMSX emulation core.
/** openMSX: the MSX emulator that aims for perfection
  *
  * Copyrights: see AUTHORS file.
  * License: GPL.
  */
namespace openmsx {

#ifdef DEBUG

#define PRT_DEBUG(mes)				\
	do {					\
		std::cout << mes << std::endl;	\
	} while (0)

#else

#define PRT_DEBUG(mes)

#endif

} // namespace openmsx

#endif // OPENMSX_H
