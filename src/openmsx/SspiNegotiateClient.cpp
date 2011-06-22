// $Id$

#ifdef _WIN32

#include "SspiNegotiateClient.h"
#include "openmsx.h"

namespace openmsx {

SspiNegotiateClient::SspiNegotiateClient(StreamWrapper& clientStream)
	: SspiPackageBase(clientStream, NEGOSSP_NAME_W)
{
}

bool SspiNegotiateClient::Authenticate()
{
	TimeStamp tsCredsExpiry;
	SECURITY_STATUS ss = AcquireCredentialsHandleW(
		NULL,
		const_cast<SEC_WCHAR*>(NEGOSSP_NAME_W),
		SECPKG_CRED_OUTBOUND,
		NULL,
		NULL,
		NULL,
		NULL,
		&hCreds,
		&tsCredsExpiry);

	DebugPrintSecurityStatus("AcquireCredentialsHandleW", ss);
	if (ss != SEC_E_OK) {
		return false;
	}

	SecBufferDesc secClientBufferDesc, secServerBufferDesc;
	SecBuffer secClientBuffer, secServerBuffer;
	InitTokenContextBuffer(&secClientBufferDesc, &secClientBuffer);
	InitTokenContextBuffer(&secServerBufferDesc, &secServerBuffer);

	std::vector<char> buffer;
	PCtxtHandle phContext = NULL;
	PSecBufferDesc psecServerBufferDesc = NULL;
	while (true) {

		ULONG fContextAttr;
		TimeStamp tsContextExpiry;
		ss = InitializeSecurityContextA(
			&hCreds,
			phContext,
			NULL,	// To use Kerberos, we'll need an SPN here
			ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_CONNECTION | ISC_REQ_STREAM,
			0,
			SECURITY_NETWORK_DREP,
			psecServerBufferDesc,
			0,
			&hContext,
			&secClientBufferDesc,
			&fContextAttr,
			&tsContextExpiry);

		DebugPrintSecurityStatus("InitializeSecurityContextA", ss);
		if (ss != SEC_E_OK && ss != SEC_I_CONTINUE_NEEDED) {
			return false;
		}

		// If we have something for the server, send it
		if (secClientBuffer.cbBuffer) {
			PRT_DEBUG("Sending " << secClientBuffer.cbBuffer << " bytes to server");
			bool ret = SendChunk(stream, secClientBuffer.pvBuffer, secClientBuffer.cbBuffer);
			ClearContextBuffers(&secClientBufferDesc);
			if (!ret) {
				PRT_DEBUG("SendChunk failed");
				return false;
			}
		}

		// SEC_E_OK means that we're done
		if (ss == SEC_E_OK) {
			DebugPrintSecurityPackageName(&hContext);
			DebugPrintSecurityPrincipalName(&hContext);
			return true;
		}

		// Receive another buffer from the server
		PRT_DEBUG("Receiving server chunk");
		bool ret = RecvChunk(stream, buffer, cbMaxTokenSize);
		if (!ret) {
			PRT_DEBUG("RecvChunk failed");
			return false;
		}
		PRT_DEBUG("Received " << buffer.size() << " bytes");

		// Another time around the loop
		secServerBuffer.cbBuffer = static_cast<unsigned long>(buffer.size());
		secServerBuffer.pvBuffer = &buffer[0];

		phContext = &hContext;
		psecServerBufferDesc = &secServerBufferDesc;
	}
}

} // namespace openmsx

#endif // _WIN32
