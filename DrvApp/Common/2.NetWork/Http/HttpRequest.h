#pragma once 

namespace ddk::net::http {
	class HttpRequest
	{
	private:
		std::map<std::string, std::string> m_requestHeader;
	public:
		HttpRequest()
		{
			m_requestHeader["Host"] = " ";
			m_requestHeader["User-Agent"] = "Mozilla / 5.0 (Windows NT 6.3; WOW64) AppleWebKit / 537.36 (KHTML, like Gecko) Chrome / 41.0.2272.118 Safari / 537.36";
			m_requestHeader["Connection"] = "close";
			m_requestHeader["Accept"] = "*/*";
			m_requestHeader["Cookie"] = " ";
		}
		~HttpRequest() {};
		std::string & operator [](const std::string &key)
		{
			auto p = m_requestHeader.find(key);
			if (p == m_requestHeader.end())
			{
				m_requestHeader[key] = "";
			}
			return m_requestHeader[key];
		}
	public:
		void getRequest(std::string &request, std::string directory)
		{
			request = std::string("GET") + " " + directory + " HTTP/1.1\r\n";
			for (auto item : m_requestHeader)
			{
				if (item.second.length() > 1)
					request += item.first + ": " + item.second + "\r\n";
			}
			request += "\r\n";
			LOG_DEBUG("%s", request.c_str());
		}
		void setReuqest(std::string key, std::string value) {
			m_requestHeader[key] = value;
		}
	};
}