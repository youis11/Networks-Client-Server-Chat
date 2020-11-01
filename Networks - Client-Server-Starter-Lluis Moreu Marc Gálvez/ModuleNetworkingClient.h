#pragma once

#include "ModuleNetworking.h"
#include <list>

#define IMGUI_COLOR_YELLOW ImVec4(1.0f,1.0f,0.0f,1.0f)
#define IMGUI_COLOR_CIAN ImVec4(0.1f,0.9f,1.0f,1.0f)
#define IMGUI_COLOR_GREEN ImVec4(0.2f,0.9f,0.5f,1.0f)
#define IMGUI_COLOR_WHITE ImVec4(1.0f,1.0f,1.0f,1.0f)
#define IMGUI_COLOR_GREY ImVec4(0.3f,0.4f,0.5f,1.0f)
#define IMGUI_COLOR_RED ImVec4(0.7f,0.f,0.f,1.0f)
#define IMGUI_COLOR_PURPLE ImVec4(0.6f,0.4f,0.8f, 1.f)

class ModuleNetworkingClient : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingClient public methods
	//////////////////////////////////////////////////////////////////////

	bool start(const char *serverAddress, int serverPort, const char *playerName);

	bool isRunning() const;



private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool update() override;

	bool gui() override;



	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	void onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet) override;

	void onSocketDisconnected(SOCKET socket) override;



	//////////////////////////////////////////////////////////////////////
	// Client state
	//////////////////////////////////////////////////////////////////////

	enum class ClientState
	{
		Stopped,
		Start,
		Logging
	};

	ClientState state = ClientState::Stopped;

	sockaddr_in serverAddress = {};
	SOCKET s = INVALID_SOCKET;

	bool has_kicked;
	bool on_drugs;
	std::string playerName;
	std::list<std::pair<std::string, ImVec4>> m_messages;
	ImVec4 textColors[7] = { IMGUI_COLOR_YELLOW, IMGUI_COLOR_CIAN, IMGUI_COLOR_GREEN, IMGUI_COLOR_WHITE, IMGUI_COLOR_GREY, IMGUI_COLOR_RED, IMGUI_COLOR_PURPLE };
};									  


 
 
