#pragma once
enum CMD
{
	CMD_LOGIN,
	CMD_LOGIN_RESULT,
	CMD_LOGOUT,
	CMD_LOGOUT_RESULT,
	CMD_ERROR,
	CMD_NEWUSER_JOIN
};
struct Netmsg_DataHeader
{
	short dataLen;
	short cmd;
};

struct Netmsg_Login :public Netmsg_DataHeader
{
	Netmsg_Login() {
		this->dataLen = sizeof(Netmsg_Login);
		this->cmd = CMD_LOGIN;
	}
	char userName[32];
	char password[32];
	char data[32];
};
struct Netmsg_LoginR :public Netmsg_DataHeader
{
	Netmsg_LoginR() {
		this->dataLen = sizeof(Netmsg_LoginR);
		this->cmd = CMD_LOGIN_RESULT;
	}
	int result;
	char data[92];
};

struct Netmsg_Logout :public Netmsg_DataHeader
{
	Netmsg_Logout() {
		this->dataLen = sizeof(Netmsg_Logout);
		this->cmd = CMD_LOGOUT;
	}
	char userName[32];
};
struct Netmsg_LogoutR :public Netmsg_DataHeader
{
	Netmsg_LogoutR() {
		this->dataLen = sizeof(Netmsg_LogoutR);
		this->cmd = CMD_LOGOUT_RESULT;
	}
	int result;
};

struct Netmsg_NewUserJoin :public Netmsg_DataHeader
{
	Netmsg_NewUserJoin() {
		this->dataLen = sizeof(Netmsg_NewUserJoin);
		this->cmd = CMD_NEWUSER_JOIN;
	}
	int sock;
};