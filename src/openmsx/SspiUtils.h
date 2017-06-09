#ifndef SSPI_UTILS_HH
#define SSPI_UTILS_HH

#ifdef _WIN32

#include <winsock2.h>
#ifdef __GNUC__
// MinGW32 requires that subauth.h be included before security.h, in order to define several things
// This differs from VC++, which only needs security.h
#include <subauth.h>
// MinGW32 does not define NEGOSSP_NAME_W anywhere. It should.
#define NEGOSSP_NAME_W  L"Negotiate"
#endif

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

#include <security.h>
#include <vector>
#include "openmsx.h"

//
// NOTE: This file MUST be kept in sync between the openmsx and openmsx-debugger projects
//

namespace openmsx {
namespace sspiutils {

const unsigned STREAM_ERROR = 0xffffffff;

class StreamWrapper
{
public:
	virtual uint32 Read(void* buffer, uint32 cb) = 0;
	virtual uint32 Write(void* buffer, uint32 cb) = 0;
};

class SspiPackageBase
{
protected:
	CredHandle hCreds;
	CtxtHandle hContext;

	StreamWrapper& stream;
	const unsigned int cbMaxTokenSize;

	SspiPackageBase(StreamWrapper& stream, const SEC_WCHAR* securityPackage);
	~SspiPackageBase();
};

// Generic access control flags, used with AccessCheck
const DWORD ACCESS_READ = 0x1;
const DWORD ACCESS_WRITE = 0x2;
const DWORD ACCESS_EXECUTE = 0x4;
const DWORD ACCESS_ALL = ACCESS_READ | ACCESS_WRITE | ACCESS_EXECUTE;

const GENERIC_MAPPING mapping = { 
	ACCESS_READ, ACCESS_WRITE, ACCESS_EXECUTE, ACCESS_ALL };

void InitTokenContextBuffer(PSecBufferDesc pSecBufferDesc, PSecBuffer pSecBuffer);
void ClearContextBuffers(PSecBufferDesc pSecBufferDesc);
void DebugPrintSecurityStatus(const char* context, SECURITY_STATUS ss);
void DebugPrintSecurityBool(const char* context, BOOL ret);
void DebugPrintSecurityPackageName(PCtxtHandle phContext);
void DebugPrintSecurityPrincipalName(PCtxtHandle phContext);
void DebugPrintSecurityDescriptor(PSECURITY_DESCRIPTOR psd);
PSECURITY_DESCRIPTOR CreateCurrentUserSecurityDescriptor();
unsigned long GetPackageMaxTokenSize(const SEC_WCHAR* package);

bool SendChunk(StreamWrapper& stream, void* buffer, uint32 cb);
bool RecvChunk(StreamWrapper& stream, std::vector<char>& buffer, uint32 cbMaxSize);

} // namespace sspiutils
} // namespace openmsx

#endif // _WIN32

#endif // SSPI_UTILS_HH
