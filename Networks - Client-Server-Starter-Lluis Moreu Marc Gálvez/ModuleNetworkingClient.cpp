#include "ModuleNetworkingClient.h"

#define IMGUI_COLOR_YELLOW ImVec4(1.0f,1.0f,0.0f,1.0f)

bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff
	// - Create the socket
	// - Create the remote address object
	// - Connect to the remote address
	// - Add the created socket to the managed list of sockets using addSocket()

	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s == INVALID_SOCKET) {
		reportError("client socket");
		return false;
	}

	serverAddress.sin_family = AF_INET; // IPv4
	serverAddress.sin_port = htons(serverPort); // Port
	const char* remoteAddrStr = serverAddressStr; // Not so remote… :-P
	inet_pton(AF_INET, remoteAddrStr, &serverAddress.sin_addr);

	if (connect(s, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
		reportError("connect");
		return false;
	}

	addSocket(s);

	// If everything was ok... change the state
	state = ClientState::Start;

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		OutputMemoryStream packet;
		packet << ClientMessage::Hello;
		packet << playerName;

		if (sendPacket(packet, s))
		{
			state = ClientState::Logging;
		}
		else 
		{
			disconnect();
			state = ClientState::Stopped;
		}
	}

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("Hello %s! Welcome to the chat :)", playerName.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Logout"))
		{
			m_messages.clear();
			m_welcome_message.clear();

			disconnect();
			state = ClientState::Stopped;
		}
		if (ImGui::BeginChild("Chat_Panel", ImVec2(ImGui::GetWindowWidth()*0.955f, ImGui::GetWindowHeight()*0.64f), true))
		{
			for (const auto& message : m_welcome_message)
			{
				ImGui::TextColored(IMGUI_COLOR_YELLOW, "%s", message.c_str());
			}
			for (const auto& message : m_messages)
			{
				//ImGui::Text("%s connected to the server...", playerName.c_str());
				ImGui::Text("%s", message.c_str());
			}
			ImGui::EndChild();
		}
		
		

		static char text[128] = "";
		if (ImGui::InputText("Press Enter", text, sizeof(text), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			OutputMemoryStream packet;
			
			packet << ClientMessage::Type;
			packet << text;
			if (!sendPacket(packet, s))
			{
				disconnect();
				state = ClientState::Stopped;
			}

			strcpy_s(text, 128, "");
			ImGui::SetKeyboardFocusHere();
		}

		ImGui::End();

	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	ServerMessage serverMessage;
	packet >> serverMessage;

	if (serverMessage == ServerMessage::Welcome) {
		std::string message;
		packet >> message;

		m_welcome_message.push_back(message);
	}
	if (serverMessage == ServerMessage::ClientDisconnection || serverMessage == ServerMessage::ClientConnection) {
		std::string message;
		packet >> message;

		m_messages.push_back(message);
	}
	if (serverMessage == ServerMessage::Type) {
		std::string message;
		packet >> message;

		m_messages.push_back(message);
	}
	else if (serverMessage == ServerMessage::NonWelcome) {
		ELOG("'%s' already exists",playerName.c_str());

		disconnect();
		state = ClientState::Stopped;
	}
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

