#pragma once
#include "WskSocket.h"
namespace ddk::net::wsk {

	class TcpClient
	{
	private:
		SOCKADDR_IN 	m_localAddress;
		SOCKADDR_IN		m_remoteAddress;
		PWSK_SOCKET		m_tcpSocket;
	public:
		TcpClient()
		{
			m_tcpSocket = nullptr;
			WSKStartup();
		}
		TcpClient(PWSK_SOCKET _socket)
		{
			this->m_tcpSocket = _socket;
		}
		~TcpClient() {
			if (m_tcpSocket)
			{
				disconnect();
				WSKCleanup();
			}
		}
		TcpClient & operator = (TcpClient &_client)
		{
			this->m_tcpSocket = _client.m_tcpSocket;
			_client.m_tcpSocket = nullptr;
			return *this;
		}
	public:
		bool connect(WCHAR *host, WCHAR *port)
		{
			if (m_tcpSocket)
				return false;
			
			auto g_TcpSocket = CreateSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, WSK_FLAG_CONNECTION_SOCKET);
			if (g_TcpSocket == NULL) {
				return false;
			}
			auto _exit = std::experimental::make_scope_exit([&]() {CloseSocket(g_TcpSocket); });

			IN4ADDR_SETANY(&m_localAddress);
			//必须绑定一个本地地址
			auto status = Bind(g_TcpSocket, (PSOCKADDR)&m_localAddress);
			if (!NT_SUCCESS(status))
			{
				return false;
			}

			UNICODE_STRING hostName;
			UNICODE_STRING portName;
			RtlInitUnicodeString(&hostName, host);
			RtlInitUnicodeString(&portName, port);
			status = ResolveName(&hostName, &portName, nullptr, &m_remoteAddress);
			if (!NT_SUCCESS(status))
			{
				return false;
			}
			status = Connect(g_TcpSocket, (PSOCKADDR)&m_remoteAddress);
			if (!NT_SUCCESS(status))
			{
				return false;
			}
			m_tcpSocket = g_TcpSocket;
			_exit.release();
			return true;
		}
		void disconnect()
		{
			DisConnect(m_tcpSocket);
			CloseSocket(m_tcpSocket);
			m_tcpSocket = nullptr;
		}
	public:
		bool send(LPCVOID buffer, ULONG size, ULONG &sizeSent)
		{
			ULONG offset = 0;
			sizeSent = 0;
			while (offset < size)
			{
				auto BytesSent = Send(m_tcpSocket, (PVOID)((ULONG_PTR)buffer + offset), size - offset, WSK_FLAG_NODELAY);
				if (BytesSent == SOCKET_ERROR)
				{
					break;
				}
				offset += BytesSent;
			}
			sizeSent = offset;
			if (sizeSent == size)
			{
				return true;
			}
			return false;
		}
		bool recv(PVOID buffer, ULONG maxSize, ULONG &recvSize)
		{
			auto ret = Receive(m_tcpSocket, buffer, maxSize, 0);
			if (ret != SOCKET_ERROR)
			{
				recvSize = ret;
				return true;
			}
			return false;
		}
		bool recv_all(PVOID buffer, ULONG maxSize, ULONG &recvSize)
		{
			ULONG offset = 0;
			while (offset < maxSize)
			{
				ULONG _rSize = 0;
				auto ret = recv((PVOID)((ULONG_PTR)buffer + offset), maxSize - offset, _rSize);
				if (!ret)
				{
					break;
				}
				offset += _rSize;
			}
			recvSize = offset;
			if (recvSize == maxSize)
			{
				return true;
			}
			return false;
		}
	};
}