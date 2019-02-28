#pragma once 

namespace ddk::net::http {
	using namespace ddk::net::wsk;
	class HttpResponse
	{
#define HTTP_CONTENT_LENGTH "content-length"
	private:
		LONG m_Pos;			//html开始的位置
		LONG m_Content;		//html开始的位置
		ULONG m_contentSize;//响应头的content-length
		ULONG m_TotalSize;	//内容+头部(m_content+m_contentSize)
		std::vector<BYTE> m_data;
		std::map<std::string, std::string> m_responseHeader;
	public:
		HttpResponse()
		{
			m_Pos = 0;
			m_TotalSize = 0;
			m_Content = 0;
			m_contentSize = 0;
		}
		~HttpResponse()	{}
		std::string & operator [](const std::string key)
		{
			auto p = m_responseHeader.find(key);
			if (p == m_responseHeader.end())
			{
				m_responseHeader[key] = "";
			}
			return m_responseHeader[key];
		}
	public://各种属性
		PVOID getData() {	
			return &m_data[0];
		}
		ULONG getTotalSize() {//加头大小
			return m_TotalSize;
		}
		PVOID getContent() {
			return &m_data[m_Content];
		}
		ULONG getContentSize() {
			return m_contentSize;
		}
	public:
		void Get(TcpClient *pClient)
		{
			m_Pos = 0;
			m_TotalSize = 0;
			while (1)
			{
				BYTE nBuf[PAGE_SIZE] = { 0 };
				ULONG RecvSize = 0;
				auto bRet = pClient->recv(nBuf, PAGE_SIZE, RecvSize);
				if (!bRet)
				{
					break;
				}
				if (RecvSize == 0)
				{
					break;
				}
				m_TotalSize += RecvSize;
				m_data.resize(m_TotalSize);
				RtlCopyMemory(&m_data[m_Pos], nBuf, RecvSize);
				m_Pos += RecvSize;
			}
			m_Pos = 0;

		}
		void Parse()
		{
			while (1)
			{
				std::string line = "";
				auto line_size = readline(line);
				//200 OK
				//DBG_PRINT("Parse %s\r\n", line.c_str());
				if (line.find("HTTP/1.1") != std::string::npos)
				{
					continue;
				}
				auto pos = line.find(":");
				if (pos == std::string::npos)
				{
					break;
				}
				std::string head_string = "";
				std::string sub = "";
				sub = line.substr(pos + 1);
				head_string = line.substr(0, pos);
				std::transform(head_string.begin(), head_string.end(), head_string.begin(), ::tolower);
				
				m_responseHeader[head_string] = sub;
			}
			if (m_responseHeader.find(HTTP_CONTENT_LENGTH) != m_responseHeader.end())
			{
				KdPrint(("get it\r\n"));
				m_Content = m_Pos;
				RtlCharToInteger(m_responseHeader[HTTP_CONTENT_LENGTH].c_str(), 10, &m_contentSize);
			}

		}
	private:
		size_t readline(std::string &line)
		{
			line = "";
			auto pos = m_Pos;
			while (pos < m_TotalSize)
			{
				char sz[2] = {};
				auto pBuf = reinterpret_cast<CHAR*>(&m_data[pos]);
				sz[0] = pBuf[0];
				sz[1] = 0;
				if (sz[0] != 0
					&& sz[0] != '\n'
					&& sz[0] != '\r')
				{
					line += std::string(sz);
				}
				if (sz[0] == '\n' || sz[0] == 0)
				{
					pos++;
					break;
				}
				pos++;
			}
			m_Pos = pos;
			return line.length();
		}
	};
}