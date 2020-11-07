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

void ModuleNetworkingServer::onSocketReceivedData(SOCKET s, const InputMemoryStream& packet)
{
	ClientMessage clientMessage;
	packet >> clientMessage;

	if (clientMessage == ClientMessage::Hello)
	{
		std::string playerName;
		packet >> playerName;

		// Set the player name of the corresponding connected socket proxy
		for (auto& c_socket : connectedSockets)
		{
			OutputMemoryStream packet;

			if (c_socket.socket == s) {
				c_socket.playerName = playerName;
				
				packet << ServerMessage::Welcome;
				packet << "**************************************************\n               WELCOME TO THE CHAT\nPlease type /help to see the available commands\n**************************************************";
			}
			else if(c_socket.playerName == playerName)
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

			if (!sendPacket(packet, c_socket.socket))
			{
				disconnect();
				state = ServerState::Stopped;
			}		

			if (c_socket.playerName == playerName)
			{
				OutputMemoryStream packete;
				packete << ServerMessage::NonWelcome;

				if (!sendPacket(packete, s))
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
		std::string playerName;
		std::string message;
		packet >> message;
		packet >> playerName;

		for (auto& c_socket : connectedSockets) {

			OutputMemoryStream packet;

			if (message == "/help" && c_socket.socket == s)
			{
				packet << ServerMessage::Help;
				std::string message = "******* Command list: *******\n/help\n/kick\n/list\n/whisper [username] [message]\n/change_name [name]\n/drugs";
				packet << message;	
				
			}
			else if (message == "/list" && c_socket.socket == s)
			{
				packet << ServerMessage::Help;
				std::string message = "******* User list *******";
				for (auto& s : connectedSockets)
				{
					message += "\n- " + s.playerName;
				}
				packet << message;
			}
			else if (message == "/play" && c_socket.socket == s)
			{
				packet << ServerMessage::Help;
				std::string message = "******* User list *******";
				for (auto& s : connectedSockets)
				{
					message += "\n- " + s.playerName;
				}
				packet << message;
			}
			else if (message == "/drugs" && c_socket.socket == s)
			{
				drugs = !drugs;
				packet << ServerMessage::Drugs;

				if (drugs)
				{
					std::string new_message = "******* DRUGS MODE ON *******\n Please type /drugs again to stop it!";
					packet << new_message;
				}
				else
				{
					std::string new_message = "******* DRUGS MODE OFF *******";
					packet << new_message;
				}
					
			}
			else if (message.find("/kick ") == 0)
			{
				std::string command = "/kick ";
				if (message.length() <= command.length())
				{
					break;
				}
				std::string playerName = message.substr(command.length());

				if (c_socket.playerName == playerName)
				{
					packet << ServerMessage::Kick;
					packet << "******** " + c_socket.playerName + " left ********";
					packet << true;
				}
				else if (c_socket.socket == s)
				{
					bool player_found = false;

					for (auto& socket : connectedSockets)
						if (socket.playerName == playerName)
							player_found = true;
						
					
					if (!player_found)
					{
						packet << ServerMessage::Error;
						std::string new_name = message.substr(6, message.length());
						packet << "******** " + new_name + " unidentified ********";
						packet << false;
					}
					
				}
				
			}
			else if (message.find("/whisper") == 0)
			{
				if (message.find("/whisper " + c_socket.playerName) == 0 || c_socket.socket == s)
				{
					packet << ServerMessage::Whisper;
					std::string whisper = "/whisper ";
					std::string argument = message.substr(whisper.length());
					std::size_t spacingIndex = argument.find(" ");
					std::string toPlayerName = argument.substr(0, spacingIndex);
					std::string new_message = argument.substr(spacingIndex + 1);
					packet << playerName + " whispered:" + new_message;
				}			
			}
			else if (message.find("/change_name") == 0 && c_socket.socket == s)
			{
				packet << ServerMessage::ChangeName;
				std::string change_name = "/change_name ";
				std::string new_name = message.substr(change_name.length(), message.length());
				packet << "*** " + c_socket.playerName + " changed to:" + new_name + " ***";
				c_socket.playerName = new_name;
				packet << c_socket.playerName;
				
			}
			else if (message.rfind("/", 0) == 0 && c_socket.socket == s)
			{
				packet << ServerMessage::Error;
				packet << "*** " + message + "is undefined ***\n    Please insert /help for more info";
			}
			else if (message.rfind("/", 0) != 0)
			{
				
				packet << ServerMessage::Type;
				packet << playerName + ": " + message;
				
				
			}
			if (!sendPacket(packet, c_socket.socket))
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

