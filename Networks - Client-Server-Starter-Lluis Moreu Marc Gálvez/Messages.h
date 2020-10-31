#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

enum class ClientMessage
{
	Hello,
	Type,
	Command,
};

enum class ServerMessage
{
	Welcome,
	NonWelcome,
	ClientConnection,
	ClientDisconnection,
	Type,
	Command,
};
