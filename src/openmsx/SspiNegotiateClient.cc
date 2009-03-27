// $Id$

#ifdef _WIN32

#include "SspiNegotiateClient.hh"
#include "openmsx.hh"

namespace openmsx {

SspiNegotiateClient::SspiNegotiateClient(StreamWrapper& clientStream)
	: SspiPackageBase(clientStream)
{
}

bool SspiNegotiateClient::Authenticate()
{
	TimeStamp tsCredsExpiry;
	SECURITY_STATUS ss = AcquireCredentialsHandleW(
		NULL,
		NEGOSSP_NAME_W,
		SECPKG_CRED_OUTBOUND,
		NULL,
		NULL,
		NULL,
		NULL,
		&hCreds,
		&tsCredsExpiry);

	PrintSecurityStatus("AcquireCredentialsHandleW", ss);
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
		
		PrintSecurityStatus("InitializeSecurityContextA", ss);
		if (ss != SEC_E_OK && ss != SEC_I_CONTINUE_NEEDED) {
			return false;
		}

		// If we have something for the server, send it
		if (secClientBuffer.cbBuffer) {
			PRT_DEBUG("Sending " << secClientBuffer.cbBuffer << " bytes to server");
			bool ret = SendChunk(stream, secClientBuffer.pvBuffer, secClientBuffer.cbBuffer);
			ClearContextBuffers(&secClientBufferDesc);
			if (!ret) {
				return false;
			}
		}

		// SEC_E_OK means that we're done
		if (ss == SEC_E_OK) {
			PrintSecurityPackageName(&hContext);
			PrintSecurityPrincipalName(&hContext);
			return true;
		}

		// Receive another buffer from the server
		PRT_DEBUG("Receiving server chunk");
		unsigned cb = RecvChunk(stream, buffer, cbMaxTokenSize);
		PRT_DEBUG("Received " << cb << " bytes");
		if (!cb) {
			return false;
		}

		// Another time around the loop
		secServerBuffer.cbBuffer = cb;
		secServerBuffer.pvBuffer = &buffer[0];

		phContext = &hContext;
		psecServerBufferDesc = &secServerBufferDesc;
	}
}

} // namespace openmsx

#endif // _WIN32
