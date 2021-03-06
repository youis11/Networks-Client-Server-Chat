#include "ModuleNetworkingClient.h"


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
	const char* remoteAddrStr = serverAddressStr; // Not so remote� :-P
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
		if (ImGui::Button("Logout") || has_kicked)
		{
			m_messages.clear();
			has_kicked = false;

			disconnect();
			state = ClientState::Stopped;
		}
		if (ImGui::BeginChild("Chat_Panel", ImVec2(ImGui::GetWindowWidth()*0.955f, ImGui::GetWindowHeight()*0.64f), true))
		{			
			for (const auto& message : m_messages)
			{
				if (!on_drugs)
				{
					ImGui::TextColored(message.second, "%s", message.first.c_str());
				}
				else
				{
					ImGui::TextColored(textColors[rand() % 7 + 1], "%s", message.first.c_str());
				}
			}
		}
		ImGui::EndChild();

		

		static char text[128] = "";
		if (ImGui::InputText("Press Enter", text, sizeof(text), ImGuiInputTextFlags_EnterReturnsTrue))
		{
			OutputMemoryStream packet;
			
			packet << ClientMessage::Type;
			packet << text;
			packet << playerName;

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

		m_messages.push_back({ message, IMGUI_COLOR_YELLOW });
	}
	else if (serverMessage == ServerMessage::ClientDisconnection) {
		std::string message;
		packet >> message;

		m_messages.push_back({ message, IMGUI_COLOR_GREY });
	}
	else if (serverMessage == ServerMessage::ClientConnection) {
		std::string message;
		packet >> message;

		m_messages.push_back({ message, IMGUI_COLOR_GREEN });
	}
	else if (serverMessage == ServerMessage::Type) {
		std::string message;
		packet >> message;

		m_messages.push_back({message, IMGUI_COLOR_WHITE });
	}
	else if (serverMessage == ServerMessage::Help) {
		std::string message;
		packet >> message;

		m_messages.push_back({message, IMGUI_COLOR_YELLOW });
	}
	else if (serverMessage == ServerMessage::Whisper) {
		std::string message;
		packet >> message;

		m_messages.push_back({message, IMGUI_COLOR_PURPLE });
	}
	else if (serverMessage == ServerMessage::Drugs) {
		std::string message;
		packet >> message;

		m_messages.push_back({ message, IMGUI_COLOR_YELLOW });
		on_drugs = !on_drugs;
	}
	else if (serverMessage == ServerMessage::RockPaperScissors) {
		std::string message;
		packet >> message;

		m_messages.push_back({ message, IMGUI_COLOR_YELLOW });
		on_drugs = !on_drugs;
	}
	else if (serverMessage == ServerMessage::ChangeName)
	{
		std::string message;
		std::string p_name;

		packet >> message;
		packet >> p_name;

		m_messages.push_back({message, IMGUI_COLOR_CIAN});
		playerName = p_name;
	}
	else if (serverMessage == ServerMessage::Kick)
	{
		std::string message;
		bool kick;

		packet >> message;
		packet >> kick;

		m_messages.push_back({ message, IMGUI_COLOR_RED });
		has_kicked = kick;
	}
	else if (serverMessage == ServerMessage::Error)
	{
		std::string message;

		packet >> message;

		m_messages.push_back({ message, IMGUI_COLOR_RED });
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

