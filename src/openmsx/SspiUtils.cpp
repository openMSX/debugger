#ifdef _WIN32

#include "SspiUtils.h"
#include <sddl.h>
#include <cassert>

//
// NOTE: This file MUST be kept in sync between the openmsx and openmsx-debugger projects
//

namespace openmsx {
namespace sspiutils {

SspiPackageBase::SspiPackageBase(StreamWrapper& userStream, const SEC_WCHAR* securityPackage)
	: stream(userStream)
	, cbMaxTokenSize(GetPackageMaxTokenSize(securityPackage))
{
	memset(&hCreds, 0, sizeof(hCreds));
	memset(&hContext, 0, sizeof(hContext));

	if (!cbMaxTokenSize) {
		// GetPackageMaxTokenSize failed
		assert(false);
	}
}

SspiPackageBase::~SspiPackageBase()
{
	DeleteSecurityContext(&hContext);
	FreeCredentialsHandle(&hCreds);
}

void InitTokenContextBuffer(PSecBufferDesc pSecBufferDesc, PSecBuffer pSecBuffer)
{
	pSecBuffer->BufferType = SECBUFFER_TOKEN;
	pSecBuffer->cbBuffer = 0;
	pSecBuffer->pvBuffer = nullptr;

	pSecBufferDesc->ulVersion = SECBUFFER_VERSION;
	pSecBufferDesc->cBuffers = 1;
	pSecBufferDesc->pBuffers = pSecBuffer;
}

void ClearContextBuffers(PSecBufferDesc pSecBufferDesc)
{
	for (ULONG i = 0; i < pSecBufferDesc->cBuffers; i ++)
	{
		FreeContextBuffer(pSecBufferDesc->pBuffers[i].pvBuffer);
		pSecBufferDesc->pBuffers[i].cbBuffer = 0;
		pSecBufferDesc->pBuffers[i].pvBuffer = nullptr;
	}
}

void DebugPrintSecurityStatus(const char* context, SECURITY_STATUS ss)
{
	(void)&context;
	(void)&ss;
#ifdef DEBUG
	switch (ss)
	{
	case SEC_E_OK:
		PRT_DEBUG(context << ": SEC_E_OK");
		break;
	case SEC_I_CONTINUE_NEEDED:
		PRT_DEBUG(context << ": SEC_I_CONTINUE_NEEDED");
		break;
	case SEC_E_INVALID_TOKEN:
		PRT_DEBUG(context << ": SEC_E_INVALID_TOKEN");
		break;
	case SEC_E_BUFFER_TOO_SMALL:
		PRT_DEBUG(context << ": SEC_E_BUFFER_TOO_SMALL");
		break;
	case SEC_E_INVALID_HANDLE:
		PRT_DEBUG(context << ": SEC_E_INVALID_HANDLE");
		break;
	case SEC_E_WRONG_PRINCIPAL:
		PRT_DEBUG(context << ": SEC_E_WRONG_PRINCIPAL");
		break;
	default:
		PRT_DEBUG(context << ": " << ss);
		break;
	}
#endif
}

void DebugPrintSecurityBool(const char* context, BOOL ret)
{
	(void)&context;
	(void)&ret;
#ifdef DEBUG
	if (ret) {
		PRT_DEBUG(context << ": true");
	} else {
		PRT_DEBUG(context << ": false - " << GetLastError());
	}
#endif
}

void DebugPrintSecurityPackageName(PCtxtHandle phContext)
{
	(void)&phContext;
#ifdef DEBUG
	SecPkgContext_PackageInfoA package;
	SECURITY_STATUS ss = QueryContextAttributesA(phContext, SECPKG_ATTR_PACKAGE_INFO, &package);
	if (ss == SEC_E_OK) {
		PRT_DEBUG("Using " << package.PackageInfo->Name << " package");
	}
#endif
}

void DebugPrintSecurityPrincipalName(PCtxtHandle phContext)
{
	(void)&phContext;
#ifdef DEBUG
	SecPkgContext_NamesA name;
	SECURITY_STATUS ss = QueryContextAttributesA(phContext, SECPKG_ATTR_NAMES, &name);
	if (ss == SEC_E_OK) {
		PRT_DEBUG("Client principal " << name.sUserName);
	}
#endif
}

void DebugPrintSecurityDescriptor(PSECURITY_DESCRIPTOR psd)
{
	(void)&psd;
#ifdef DEBUG
	char* sddl;
	BOOL ret = ConvertSecurityDescriptorToStringSecurityDescriptorA(
		psd,
		SDDL_REVISION,
		OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION |
		DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION,
		&sddl,
		nullptr);
	if (ret) {
		PRT_DEBUG("SecurityDescriptor: " << sddl);
		LocalFree(sddl);
	}
#endif
}

// If successful, caller must free the results with LocalFree()
// If unsuccessful, returns null
PTOKEN_USER GetProcessToken()
{
	PTOKEN_USER pToken = nullptr;

	HANDLE hProcessToken;
	BOOL ret = OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hProcessToken);
	DebugPrintSecurityBool("OpenProcessToken", ret);
	if (ret) {

		DWORD cbToken;
		ret = GetTokenInformation(hProcessToken, TokenUser, nullptr, 0, &cbToken);
		assert(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER && cbToken);

		pToken = (TOKEN_USER*)LocalAlloc(LMEM_ZEROINIT, cbToken);
		if (pToken) {
			ret = GetTokenInformation(hProcessToken, TokenUser, pToken, cbToken, &cbToken);
			DebugPrintSecurityBool("GetTokenInformation", ret);
			if (!ret) {
				LocalFree(pToken);
				pToken = nullptr;
			}
		}

		CloseHandle(hProcessToken);
	}

	return pToken;
}

// If successful, caller must free the results with LocalFree()
// If unsuccessful, returns null
PSECURITY_DESCRIPTOR CreateCurrentUserSecurityDescriptor()
{
	PSECURITY_DESCRIPTOR psd = nullptr;
	PTOKEN_USER pToken = GetProcessToken();
	if (pToken) {
		PSID pUserSid = pToken->User.Sid;
		const DWORD cbEachAce = sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD);
		const DWORD cbACL = sizeof(ACL) + cbEachAce + GetLengthSid(pUserSid);

		// Allocate the SD and the ACL in one allocation, so we only have one buffer to manage
		// The SD structure ends with a pointer, so the start of the ACL will be well aligned
		BYTE* buffer = (BYTE*)LocalAlloc(LMEM_ZEROINIT, SECURITY_DESCRIPTOR_MIN_LENGTH + cbACL);
		if (buffer) {
			psd = (PSECURITY_DESCRIPTOR)buffer;
			PACL pacl = (PACL)(buffer + SECURITY_DESCRIPTOR_MIN_LENGTH);
			PACCESS_ALLOWED_ACE pUserAce;
			if (InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION) &&
				InitializeAcl(pacl, cbACL, ACL_REVISION) &&
				AddAccessAllowedAce(pacl, ACL_REVISION, ACCESS_ALL, pUserSid) &&
				SetSecurityDescriptorDacl(psd, TRUE, pacl, FALSE) &&
				// Need to set the Group and Owner on the SD in order to use it with AccessCheck()
				GetAce(pacl, 0, (void**)&pUserAce) &&
				SetSecurityDescriptorGroup(psd, &pUserAce->SidStart, FALSE) &&
				SetSecurityDescriptorOwner(psd, &pUserAce->SidStart, FALSE))
			{
				buffer = nullptr;
			} else {
				psd = nullptr;
			}

			LocalFree(buffer);
		}

		LocalFree(pToken);
	}

	if (psd) {
		assert(IsValidSecurityDescriptor(psd));
		DebugPrintSecurityDescriptor(psd);
	}

	return psd;
}

unsigned long GetPackageMaxTokenSize(const SEC_WCHAR* package)
{
	PSecPkgInfoW pkgInfo;
	SECURITY_STATUS ss = QuerySecurityPackageInfoW(const_cast<SEC_WCHAR*>(package), &pkgInfo);
	DebugPrintSecurityStatus("QuerySecurityPackageInfoW", ss);
	if (ss != SEC_E_OK) {
		return 0;
	}

	unsigned long cbMaxToken = pkgInfo->cbMaxToken;
	FreeContextBuffer(pkgInfo);
	return cbMaxToken;
}

bool Send(StreamWrapper& stream, void* buffer, uint32_t cb)
{
	uint32_t sent = 0;
	while (sent < cb)
	{
		uint32_t ret = stream.Write((char*)buffer + sent, cb - sent);
		if (ret == STREAM_ERROR) {
			return false;
		}
		sent += ret;
	}
	return true;
}

bool SendChunk(StreamWrapper& stream, void* buffer, uint32_t cb)
{
	uint32_t nl = htonl(cb);
	if (!Send(stream, &nl, sizeof(nl))) {
		return false;
	}
	return Send(stream, buffer, cb);
}

bool Recv(StreamWrapper& stream, void* buffer, uint32_t cb)
{
	uint32_t recvd = 0;
	while (recvd < cb) {
		uint32_t ret = stream.Read((char*)buffer + recvd, cb - recvd);
		if (ret == STREAM_ERROR) {
			return false;
		}
		recvd += ret;
	}

	return true;
}

bool RecvChunkSize(StreamWrapper& stream, uint32_t* pcb)
{
	uint32_t cb;
	bool ret = Recv(stream, &cb, sizeof(cb));
	if (ret) {
		*pcb = ntohl(cb);
	}
	return ret;
}

bool RecvChunk(StreamWrapper& stream, std::vector<char>& buffer, uint32_t cbMaxSize)
{
	uint32_t cb;
	if (!RecvChunkSize(stream, &cb) || cb > cbMaxSize) {
		return false;
	}
	buffer.resize(cb);
	if (!Recv(stream, &buffer[0], cb)) {
		return false;
	}
	return true;
}

} // namespace sspiutils
} // namespace openmsx

#endif
