#pragma once

namespace ddk::net::http {
	class HttpClient
	{
	//在HttpRequest中已经对User-Agent、Connection、Accept进行设置
	private:
		ddk::net::wsk::TcpClient m_tcp;
		HttpRequest m_httpRequest;
	public:
		HttpClient() {

		}
		~HttpClient() {
			m_tcp.disconnect();
		}
		HttpRequest &getRequest() {
			return m_httpRequest;
		}
	public:
		bool open(WCHAR *host)
		{	//连接
			std::wstring ws;
			ws = host;
			m_httpRequest["Host"] = std::string(ws.begin(), ws.end());
			auto b = m_tcp.connect(host, L"80");
			return b;
		}
		bool get(std::string uri, HttpResponse &httpResponse)
		{	//get请求
			std::string s_requset;
			m_httpRequest.getRequest(s_requset, uri);
			KdPrint(("http %s\r\n", s_requset.c_str()));
			PVOID send_buff = malloc(s_requset.length() + MAX_PATH);
			if (send_buff)
			{
				ULONG sendSize = (ULONG)s_requset.length();
				ULONG sent = 0;
				RtlCopyMemory(send_buff, s_requset.c_str(), sendSize);
				auto b = m_tcp.send(send_buff, sendSize, sent);
				if (b)
				{
					//开始http收包处理
					httpResponse.Get(&m_tcp);
					httpResponse.Parse();
					return true;
				}
			}
			return false;
		}
	};
}