/// \file
/// \brief Contains HTTPConnection, used to communicate with web servers
///
/// This file is part of RakNet Copyright 2008 Kevin Jenkins.
///
/// Usage of RakNet is subject to the appropriate license agreement.
/// Creative Commons Licensees are subject to the
/// license found at
/// http://creativecommons.org/licenses/by-nc/2.5/
/// Single application licensees are subject to the license found at
/// http://www.jenkinssoftware.com/SingleApplicationLicense.html
/// Custom license users are subject to the terms therein.
/// GPL license users are subject to the GNU General Public
/// License as published by the Free
/// Software Foundation; either version 2 of the License, or (at your
/// option) any later version.

#if _RAKNET_SUPPORT_HTTPConnection==1

#include "TCPInterface.h"
#include "HTTPConnection.h"
#include "RakSleep.h"
#include "RakString.h"
#include "RakAssert.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

using namespace RakNet;

HTTPConnection::HTTPConnection() : connectionState(CS_NONE)
{
	tcp = 0;
}

void HTTPConnection::Init(TCPInterface* _tcp, const char *_host, unsigned short _port)
{
	tcp = _tcp;
	host = _host;
	port = _port;
}

void HTTPConnection::Post(const char *remote_path, const char *data, const char *_contentType)
{
	OutgoingPost op;
	op.contentType = _contentType;
	op.data = data;
	op.remotePath = remote_path;
	outgoingPosts.Push(op, __FILE__, __LINE__);
}

bool HTTPConnection::HasBadResponse(int *code, RakNet::RakString *data)
{
	if (badResponses.IsEmpty())
		return false;
	if (code)
		*code = badResponses.Peek().code;
	if (data)
		*data = badResponses.Pop().data;
	return true;
}
void HTTPConnection::CloseConnection()
{
	if (incomingData.IsEmpty() == false)
		results.Push(incomingData, __FILE__, __LINE__);
	incomingData.Clear();
	tcp->CloseConnection(server);
	connectionState = CS_NONE;
}
void HTTPConnection::Update(void)
{
	SystemAddress sa;
	sa = tcp->HasCompletedConnectionAttempt();
	while (sa != UNASSIGNED_SYSTEM_ADDRESS) {
		connectionState = CS_CONNECTED;
		server = sa;
		sa = tcp->HasCompletedConnectionAttempt();
	}
	sa = tcp->HasFailedConnectionAttempt();
	while (sa != UNASSIGNED_SYSTEM_ADDRESS) {
		CloseConnection();
		sa = tcp->HasFailedConnectionAttempt();
	}
	sa = tcp->HasLostConnection();
	while (sa != UNASSIGNED_SYSTEM_ADDRESS) {
		CloseConnection();
		sa = tcp->HasLostConnection();
	}
	switch (connectionState) {
		case CS_NONE: {
				if (outgoingPosts.IsEmpty())
					return;
				server = tcp->Connect(host, port, false);
				connectionState = CS_CONNECTING;
			}
			break;
		case CS_CONNECTING: {
			}
			break;
		case CS_CONNECTED: {
				if (outgoingPosts.IsEmpty()) {
					CloseConnection();
					return;
				}
#if defined(OPEN_SSL_CLIENT_SUPPORT)
				tcp->StartSSLClient(server);
#endif
				currentProcessingRequest = outgoingPosts.Pop();
				RakString request("POST %s HTTP/1.0\r\n"
				                  "Host: %s:%i\r\n"
				                  "Content-Type: %s\r\n"
				                  "Content-Length: %u\r\n"
				                  "\r\n"
				                  "%s",
				                  currentProcessingRequest.remotePath.C_String(),
				                  host.C_String(),
				                  port,
				                  currentProcessingRequest.contentType.C_String(),
				                  (unsigned) currentProcessingRequest.data.GetLength(),
				                  currentProcessingRequest.data.C_String());
				tcp->Send(request.C_String(), (unsigned int) request.GetLength(), server, false);
				connectionState = CS_PROCESSING;
			}
			break;
		case CS_PROCESSING: {
			}
	}
}
bool HTTPConnection::HasRead(void) const
{
	return results.IsEmpty() == false;
}
RakString HTTPConnection::Read(void)
{
	if (results.IsEmpty())
		return RakString();
	RakNet::RakString resultStr = results.Pop();
	const char *start_of_body = strpbrk(resultStr.C_String(), "\001\002\003%");
	if (! start_of_body)
		return RakString();
	return RakNet::RakString::NonVariadic(start_of_body);
}
SystemAddress HTTPConnection::GetServerAddress(void) const
{
	return server;
}
void HTTPConnection::ProcessTCPPacket(Packet *packet)
{
	RakAssert(packet);
	// read all the packets possible
	if (packet->systemAddress == server) {
		if (incomingData.GetLength() == 0) {
			int response_code = atoi((char *)packet->data + strlen("HTTP/1.0 "));
			if (response_code > 299) {
				badResponses.Push(BadResponse(packet->data, response_code), __FILE__, __LINE__);
				CloseConnection();
				return;
			}
		}
		RakNet::RakString incomingTemp = RakNet::RakString::NonVariadic((const char*) packet->data);
		incomingTemp.URLDecode();
		incomingData += incomingTemp;
		RakAssert(strlen((char *)packet->data) == packet->length); // otherwise it contains Null bytes
		const char *start_of_body = strstr(incomingData, "\r\n\r\n");
		// besides having the server close the connection, they may
		// provide a length header and supply that many bytes
		if (start_of_body && connectionState == CS_PROCESSING) {
			long length_of_headers = (long)(start_of_body + 4 - incomingData.C_String());
			const char *length_header = strstr(incomingData, "\r\nLength: ");
			if (length_header) {
				long length = atol(length_header + 10) + length_of_headers;
				if ((long) incomingData.GetLength() >= length)
					CloseConnection();
			}
		}
	}
}

bool HTTPConnection::IsBusy(void) const
{
	return connectionState != CS_NONE;
}

int HTTPConnection::GetState(void) const
{
	return connectionState;
}


HTTPConnection::~HTTPConnection(void)
{
	if (tcp)
		tcp->CloseConnection(server);
}


#endif // _RAKNET_SUPPORT_*
