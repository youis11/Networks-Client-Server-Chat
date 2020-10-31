#include "ModuleNetworkingServer.h"

//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::start(int port)
{
	// TODO(jesus): TCP listen socket stuff
	// - Create the listenSocket
	// - Set address reuse
	// - Bind the socket to a local interface
	// - Enter in listen mode
	// - Add the listenSocket to the managed list of sockets using addSocket()

	listenSocket = socket(AF_INET, SOCK_STREAM, 0);

	if (listenSocket == INVALID_SOCKET) {
		reportError("server socket");
		return false;
	}

	struct sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET; // IPv4
	bindAddr.sin_port = htons(port); // Port
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY; // Any local IP address

	if(bind(listenSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) == SOCKET_ERROR){
		reportError("bind");
		return false;
	}

	if (listen(listenSocket, 1) == SOCKET_ERROR) {
		reportError("listen");
		return false;
	}
	
	addSocket(listenSocket);

	state = ServerState::Listening;

	return true;
}

bool ModuleNetworkingServer::isRunning() const
{
	return state != ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Module virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::update()
{
	return true;
}

bool ModuleNetworkingServer::gui()
{
	if (state != ServerState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Server Window");

		Texture *tex = App->modResources->server;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("List of connected sockets:");

		for (auto &connectedSocket : connectedSockets)
		{
			ImGui::Separator();
			ImGui::Text("Socket ID: %d", connectedSocket.socket);
			ImGui::Text("Address: %d.%d.%d.%d:%d",
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b1,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b2,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b3,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b4,
				ntohs(connectedSocket.address.sin_port));
			ImGui::Text("Player name: %s", connectedSocket.playerName.c_str());
		}

		ImGui::End();
	}

	return true;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::isListenSocket(SOCKET socket) const
{
	return socket == listenSocket;
}

void ModuleNetworkingServer::onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress)
{
	// Add a new connected socket to the list
	ConnectedSocket connectedSocket;
	connectedSocket.socket = socket;
	connectedSocket.address = socketAddress;
	connectedSockets.push_back(connectedSocket);
}

void ModuleNetworkingServer::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	ClientMessage clientMessage;
	packet >> clientMessage;

	if (clientMessage == ClientMessage::Hello)
	{
		std::string playerName;
		packet >> playerName;

		// Set the player name of the corresponding connected socket proxy
		for (auto& connectedSocket : connectedSockets)
		{
			OutputMemoryStream packet;

			if (connectedSocket.socket == socket) {
				connectedSocket.playerName = playerName;
				
				packet << ServerMessage::Welcome;
				packet << "**************************************************\n               WELCOME TO THE CHAT\nPlease type /help to see the available commands\n**************************************************";
			}
			else if(connectedSocket.playerName == playerName)
			{
				packet << ServerMessage::ClientConnection;
				std::string message = "** " + playerName + " couldn't join due to duplicated names **";
				packet << message;
			}
			else
			{
				packet << ServerMessage::ClientConnection;
				std::string message = "******* " + playerName + " joined *******";
				packet << message;
			}

			if (!sendPacket(packet, connectedSocket.socket))
			{
				disconnect();
				state = ServerState::Stopped;
			}		

			if (connectedSocket.playerName == playerName)
			{
				OutputMemoryStream packete;
				packete << ServerMessage::NonWelcome;

				if (!sendPacket(packete, socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}

				return;
			}
		}
	}
	else if (clientMessage == ClientMessage::Type)
	{
		std::string message;
		packet >> message;

		for (auto& connectedSocket : connectedSockets) {

			OutputMemoryStream packet;
			packet << ServerMessage::Type;
			if (message == "/help")
			{
				std::string message = "******* Command list: *******\n/help\n/kick\/list\n/whisper [username] [message]\n/change_name [name]";
				packet << message;
			}
			else if (message == "/list")
			{
				std::string message = "******* User list *******";
				for (auto& s : connectedSockets)
				{
					message += "\n- " + s.playerName;
				}
				packet << message;
			}
			else if (message.find("/kick") == 0)
			{
				for (auto& s : connectedSockets)
				{
					if (message.find(s.playerName) == 0)
					{
						packet << "******** " + s.playerName + " left ********";
					}
				}

				//Pasar un paquete al cliente para que chape el negocio
			}
			else if (message.find("/whisper") == 0)
			{
				for (auto& s : connectedSockets)
				{
					if (message.find(s.playerName) == 0)
					{

					}
				}
			}
			else if (message.find("/change_name") == 0)
			{
				std::string new_name = message.substr(12, message.length());
				packet << "******** " + connectedSocket.playerName + " changed his name to:" + new_name + " ********";
				connectedSocket.playerName = new_name;
			}
			else
			{
				packet << connectedSocket.playerName + ": " + message;
			}
			if (!sendPacket(packet, connectedSocket.socket))
			{
				disconnect();
				state = ServerState::Stopped;

				break;
			}
		}
	}
}

void ModuleNetworkingServer::onSocketDisconnected(SOCKET socket)
{
	// Remove the connected socket from the list
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		if ((*it).socket == socket) {

			std::string playerName = (*it).playerName;
			connectedSockets.erase(it);

			OutputMemoryStream packet;

			for (const auto& connectedSocket : connectedSockets) {

				packet << ServerMessage::ClientDisconnection;
				std::string message = "******** " + playerName + " left ********";
				packet << message;

				if (!sendPacket(packet, connectedSocket.socket))
				{
					disconnect();
					state = ServerState::Stopped;
				}

			}

			break;
		}
	}
}

