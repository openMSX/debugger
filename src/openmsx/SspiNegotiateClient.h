#ifndef SSPI_NEGOTIATE_CLIENT_H
#define SSPI_NEGOTIATE_CLIENT_H

#ifdef _WIN32

#include "SspiUtils.h"

namespace openmsx {

using namespace sspiutils;

class SspiNegotiateClient : public SspiPackageBase
{
public:
	SspiNegotiateClient(StreamWrapper& clientStream);
	bool Authenticate();
};

} // namespace openmsx

#endif // _WIN32

#endif // SSPI_NEGOTIATE_CLIENT_H
