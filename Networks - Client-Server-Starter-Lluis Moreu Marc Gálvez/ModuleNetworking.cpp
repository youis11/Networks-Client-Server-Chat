#include "Networks.h"
#include "ModuleNetworking.h"
#include "list"

static uint8 NumModulesUsingWinsock = 0;



void ModuleNetworking::reportError(const char* inOperationDesc)
{
	LPVOID lpMsgBuf;
	DWORD errorNum = WSAGetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	ELOG("Error %s: %d- %s", inOperationDesc, errorNum, lpMsgBuf);
}

void ModuleNetworking::disconnect()
{
	for (SOCKET socket : sockets)
	{
		shutdown(socket, 2);
		closesocket(socket);
	}

	sockets.clear();
}

bool ModuleNetworking::init()
{
	if (NumModulesUsingWinsock == 0)
	{
		NumModulesUsingWinsock++;

		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(version, &data) != 0)
		{
			reportError("ModuleNetworking::init() - WSAStartup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::preUpdate()
{
	if (sockets.empty()) return true;

	// NOTE(jesus): You can use this temporary buffer to store data from recv()
	const uint32 incomingDataBufferSize = Kilobytes(1);
	byte incomingDataBuffer[incomingDataBufferSize];
	InputMemoryStream packet;

	// TODO(jesus): select those sockets that have a read operation available
	fd_set readSet;
	FD_ZERO(&readSet);

	for (auto s : sockets) {
		FD_SET(s, &readSet);
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	if (select(0, &readSet, nullptr, nullptr, &timeout) == SOCKET_ERROR) {
		reportError("SELECT");
		return false;
	}
	std::list<SOCKET> disconnectedSockets;

	for (auto s : sockets)
	{
		if (FD_ISSET(s, &readSet)) {
			if (isListenSocket(s)) { // Is the server socket
				sockaddr_in remoteAddr;
				int remoteAddrSize = sizeof(remoteAddr);
				SOCKET s2 = accept(s, (struct sockaddr*)&remoteAddr, &remoteAddrSize);
				if (s2 == SOCKET_ERROR)
				{
					reportError("ACCEPT");
				}
				else
				{
					onSocketConnected(s2, remoteAddr);
					addSocket(s2);
				}
			}
			else { // Is a client socket
				memset(incomingDataBuffer, '\0', incomingDataBufferSize);
				int bytesRead = recv(s, packet.GetBufferPtr(), packet.GetCapacity(), 0);
				if (bytesRead == SOCKET_ERROR)
				{
					reportError("RECV");
					disconnectedSockets.push_back(s);
				}
				else if (bytesRead == 0) {
					disconnectedSockets.push_back(s);
				}
				else
				{
					packet.SetSize((uint32)bytesRead);
					onSocketReceivedData(s, packet);
				}
			}
		}
	}


	// TODO(jesus): for those sockets selected, check wheter or not they are
	// a listen socket or a standard socket and perform the corresponding
	// operation (accept() an incoming connection or recv() incoming data,
	// respectively).
	// On accept() success, communicate the new connected socket to the
	// subclass (use the callback onSocketConnected()), and add the new
	// connected socket to the managed list of sockets.
	// On recv() success, communicate the incoming data received to the
	// subclass (use the callback onSocketReceivedData()).

	// TODO(jesus): handle disconnections. Remember that a socket has been
	// disconnected from its remote end either when recv() returned 0,
	// or when it generated some errors such as ECONNRESET.
	// Communicate detected disconnections to the subclass using the callback
	// onSocketDisconnected().

	// TODO(jesus): Finally, remove all disconnected sockets from the list
	// of managed sockets.

	for (const auto& s : disconnectedSockets) {		
		onSocketDisconnected(s);
		sockets.erase(std::find(sockets.begin(), sockets.end(), s));
	}

	return true;
}

bool ModuleNetworking::cleanUp()
{
	disconnect();

	NumModulesUsingWinsock--;
	if (NumModulesUsingWinsock == 0)
	{

		if (WSACleanup() != 0)
		{
			reportError("ModuleNetworking::cleanUp() - WSACleanup");
			return false;
		}
	}

	return true;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}

bool ModuleNetworking::sendPacket(const OutputMemoryStream & packet, SOCKET socket)
{
	int result = send(socket, packet.GetBufferPtr(), packet.GetSize(), 0);
	if (result == SOCKET_ERROR)
	{
		reportError("send");
		return false;
	}
	return true;
}