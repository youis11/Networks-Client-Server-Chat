#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

enum class ClientMessage
{
	Hello,
	Type
};

enum class ServerMessage
{
	Welcome,
	NonWelcome,
	ClientConnection,
	ClientDisconnection,
	Type,
	ChangeName,
	Help,
	Kick
};
