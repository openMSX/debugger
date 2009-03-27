// $Id$

#ifndef SSPI_NEGOTIATE_CLIENT_HH
#define SSPI_NEGOTIATE_CLIENT_HH

#ifdef _WIN32

#include "SspiUtils.hh"

namespace openmsx {

class SspiNegotiateClient : public SspiPackageBase
{
public:
	SspiNegotiateClient(StreamWrapper& clientStream);
	bool Authenticate();
};

} // namespace openmsx

#endif // _WIN32

#endif // SSPI_NEGOTIATE_CLIENT_HH
