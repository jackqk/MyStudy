#pragma once
#include "WskSocket.h"

namespace ddk::net::wsk {
	class UdpClient
	{
	private:
		SOCKADDR_IN 	LocalAddress;
		PWSK_SOCKET		udpSocket;
	public:
		UdpClient()
		{
			udpSocket = nullptr;
			WSKStartup();

		}
		UdpClient(USHORT port)
		{
			udpSocket = nullptr;
			WSKStartup();
			create(port);
		}
		~UdpClient()
		{
			if (udpSocket)
			{
				CloseSocket(udpSocket);
				udpSocket = nullptr;
				WSKCleanup();
			}
		}
		UdpClient & operator =(UdpClient &_client)
		{
			this->LocalAddress = _client.LocalAddress;
			this->udpSocket = _client.udpSocket;
			_client.udpSocket = nullptr;
			return *this;
		}
	public:
		bool create(USHORT port)
		{
			if (udpSocket)
			{
				return false;
			}
			auto g_UdpSocket = CreateSocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, WSK_FLAG_DATAGRAM_SOCKET);
			if (g_UdpSocket == NULL) {
				return false;
			}
			IN4ADDR_SETANY(&LocalAddress);
			LocalAddress.sin_port = RtlUshortByteSwap(port);
			// Bind Required
			auto status = Bind(g_UdpSocket, (PSOCKADDR)&LocalAddress);
			if (!NT_SUCCESS(status)) {
				CloseSocket(g_UdpSocket);
				return false;
			}
			udpSocket = g_UdpSocket;
			return true;
		}
		bool sendto(WCHAR *host, WCHAR *port, PVOID buffer, ULONG buf_size,	ULONG &sentSize)
		{
			SOCKADDR_IN		RemoteAddress;
			UNICODE_STRING hostName;
			UNICODE_STRING portName;
			RtlInitUnicodeString(&hostName, host);
			RtlInitUnicodeString(&portName, port);
			auto status = ResolveName(&hostName, &portName, nullptr, &RemoteAddress);
			if (!NT_SUCCESS(status))
			{
				return false;
			}
			sentSize = SendTo(udpSocket, PVOID(buffer), buf_size, (PSOCKADDR)&RemoteAddress);
			if (sentSize == SOCKET_ERROR)
			{
				return false;
			}
			return true;
		}
		bool recvfrom(PVOID buffer, ULONG maxSize, ULONG &recvSize, PSOCKADDR __out_opt RemoteAddress = NULL)
		{
			recvSize = ReceiveFrom(udpSocket, buffer, maxSize, RemoteAddress, nullptr);
			if (recvSize != SOCKET_ERROR)
			{
				return true;
			}
			return false;
		}
	};
}