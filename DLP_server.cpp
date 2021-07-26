// DLP_server.cpp: ���������� ����� ����� ��� ����������� ����������.
//

#include "stdafx.h"

#include "cpprest/http_client.h"
#include "cpprest/http_listener.h"
#include "cpprest/json.h"
#include "cpprest/filestream.h"
#include "cpprest/containerstream.h"
#include "cpprest/producerconsumerstream.h"
#include <cpprest/interopstream.h>

#include <agents.h>
#include <locale>
#include <ctime>
#include <ostream>
#include <istream>

using namespace web;
using namespace http;
using namespace utility;
using namespace concurrency::streams;
using namespace http::experimental::listener;

class DLPServer
{
	public:
		DLPServer()
		{
			example_file_operations();
			listener = http_listener(U("http://localhost:12345"));//TODO ������� ����
			listener.support(methods::GET, // ��� ��������-GET
				std::bind(&DLPServer::handle_get, this, std::placeholders::_1));// ������������ ����� ������� handle_get � 1 ����������
			listener.support(methods::POST,
				std::bind(&DLPServer::handle_post, this, std::placeholders::_1));
			listener.open().wait(); // ������ �������
			
		}

		void example_file_operations()
		{
			try
			{
				//����������� ��������� ����� �� �������� �����, �� ��������� get �������� ����� ��� �������
				concurrency::streams::istream fileIStream =
					concurrency::streams::file_stream<uint8_t>::
					open_istream(U("DataBase.txt")).get(); 

				std::string second_name("");
				std::string first_name("");
				std::string patronymic("");

				/* ������ ���������� �� ����� �� �����������, ���������� - ������ ��������� ����� fileStream.read_line */
				
				Concurrency::streams::stringstreambuf tmp_stream_buffer2;

				fileIStream.read_to_delim(tmp_stream_buffer2, ' '/*������-�����������*/).get();
				
				// ����� �������� � async_iostream (��. ����) ����� ������ ������������� �����
				auto intermediate_async_stream2 =
					Concurrency::streams::stringstream::
					open_istream(tmp_stream_buffer2.collection()); 

				// ������ ������ �����, async_iostream, ��������� �������� ��� � ���������� ��� std-��������,
				// �� ����������, ��������� � � ����� ����������� �������� cpprestsdk
				Concurrency::streams::async_istream<char>
					async_inout_stream2(intermediate_async_stream2.streambuf());

				async_inout_stream2 >> second_name; // "������"

				/* ������ ������ ����������� ����������� ����� � ����� � ����������� ������ � ��� ����� ������,
				�������������: cpprestsdk/Release/tests/functional/streams/stdstream_tests.cpp*/

				concurrency::streams::stringstreambuf tmp_stream_buffer; // ����������� ����� ��� ������ � ���������� ����� 

				fileIStream.read_to_end(tmp_stream_buffer).get(); // ���������� ����������� ����� � �����

				// ����� �������� � async_iostream (��. ����) ����� ������ ������������� �����
				auto intermediate_async_stream =
					Concurrency::streams::stringstream::
					open_istream(tmp_stream_buffer.collection()); 

				// ������ ������ �����, async_iostream, ��������� �������� ��� � ���������� ��� std-��������,
				// �� ����������, ��������� � � ����� ����������� �������� cpprestsdk
				Concurrency::streams::async_istream<char>
					async_inout_stream(intermediate_async_stream.streambuf());

				async_inout_stream >> first_name >> patronymic; // "����" "��������"

				fileIStream.close();

				/* ����� � ���� (�� �������� � https://casablanca.codeplex.com/wikipage?title=Asynchronous%20Streams,
				https://github.com/Microsoft/cpprestsdk/wiki/Getting-Started-Tutorial) */

				concurrency::streams::ostream fileOStream =
					concurrency::streams::file_stream<uint8_t>::
					open_ostream(U("DataBase.txt")).get();

				// ������� �������� ����� � ������� �����/������
				Concurrency::streams::async_ostream<char>
					async_inout_stream3(fileOStream.streambuf());

				async_inout_stream3 << second_name << ' ' << first_name << ' ' << patronymic << "\r\n"; // ������ ������, ��������� �� ������������ �����
				async_inout_stream3 << "������" << ' ' << "ϸ��" << ' ' << "��������" << "\r\n"; // ������ ����� �������

				fileOStream.close();

			}
			catch (const http_exception &e)
			{
				std::cerr << e.what();
				throw e;
			}
			catch (const std::exception &e)
			{
				std::cerr << e.what() << std::endl;
			}
		}

		http_listener listener;

		concurrency::streams::container_buffer<std::string> async_string_buffer;
		//concurrency::streams::stdio_istream<char> abc;
		// 
		//concurrency::streams::basic_istream<char> async_input_stream();// async_string_buffer.create_istream());// ������������� ����������� ����� �����
		//std::basic_istream<char> standard_istream;// ����������� ����� ����� ��� ������������� � ����� �����������
		//concurrency::streams::basic_ostream<char> async_output_stream;// ������������� ����������� ����� ������
		//std::basic_ostream<char> standard_ostream;// ����������� ����� ������ ��� ������������� � ����� �����������

		void handle_get(web::http::http_request message) // http_request �������� �� �������
		{

			std::wcout << U("GET request") << std::endl;

			try
			{
				message.extract_json()
					.then(
						[](json::value jsonValue)
					{
						std::wstring tmp = jsonValue.serialize();
						std::wcout << jsonValue.serialize() << std::endl;
						/*for (auto iterArray = jsonValue.cbegin(); iterArray != jsonValue.cend(); ++iterArray)
						{
							const json::value &arrayValue = iterArray->second;

							for (auto iterInner = arrayValue.cbegin(); iterInner != arrayValue.cend(); ++iterInner)
							{
								const json::value &propertyName = iterInner->first;
								const json::value &propertyValue = iterInner->second;

								std::wcout
									<< L"Property: " << propertyName.as_string()
									<< L", Value: " << propertyValue.to_string()
									<< std::endl;

							}

							std::wcout << std::endl;
						}*/
					});
			}
			catch (http_exception const & e)
			{
				std::wcout << e.what() << std::endl;
			}
	
			/*std::wcout << L"������� ������ GET" << std::endl;
			message.reply(status_codes::OK)
				.then([](pplx::task<void> t) {
				std::wcout << L"��������� ����� �������" << std::endl;
			});*/
		}

		void handle_post(web::http::http_request message)
		{
			std::wcout << U("POST request") << std::endl;

			if (message.request_uri() == U("/dlpapi/register"))
			{
				try
				{
					message.extract_json()
					.then(
					[this, message](json::value jsonValue) // ������ �������!!!
					{
						
						//std::wstring tmp = jsonValue.serialize();
						//std::wcout << jsonValue.serialize() << std::endl;// ������� json-������ � ������, �� �������� �
						std::wcout << jsonValue.at(U("password")).serialize() << std::endl;
						std::wcout << jsonValue.at(U("login")).serialize() << std::endl;
						std::wcout << jsonValue.at(U("captcha")).serialize() << std::endl;
						
						std::string line = async_string_buffer.collection();//�������� ���������� ����� ����� (����� �����) � ������

						std::wstring wline = std::wstring(async_string_buffer.collection().begin(),
							async_string_buffer.collection().end()); // �������������� � 16-������ ������

						json::value replyData; // ������ json-������, ����� - ����������� �����������
						replyData[U("string")] = json::value::string(wline);


						http_response response;
						response.set_reason_phrase(U("Access granted"));
						response.set_status_code(status_codes::OK);
						response.set_body(replyData);

						message.reply(response).wait();// ������ ������ (������� task �� �������� response ������� �������)
						
						
					});
				}
				catch (http_exception const & e)
				{
					std::wcout << e.what() << std::endl;
				}
				// ���� ������� ������������ ����� ���������������� � ������� - �����������
				// ���� ��� - ��������� �� ���������� �����������
			}
			else if (message.request_uri() == U("/dlpapi/auth"))
			{
				std::wcout << L"Authentification request" << std::endl;
				// ���� ������������ ��������������� � ������ ����� �����-������ - ������ ��� �����
				// ���� � ������ ���� �����-������ ����������� � ������� - 
			}
			else if (message.request_uri() == U("/dlpapi/access"))
			{
				std::wcout << L"Database request access" << std::endl;
				// ���� ������������ ���������������� - ������ ��� ����������� ������
				// ���� ����������� �� �������� - 

			}
			else
			{
				std::wcout << L"!!!UNKNOWN REQUEST!!!" << std::endl;
			}

		}

		~DLPServer()
		{
			listener.close().wait();

			
			//async_string_buffer.close();
			// ������ ���������
			//fileStream.close(); // �������� �����

		}
};


int main()
{
	setlocale(LC_ALL, "Russian");
	DLPServer server;
	
	std::string line;
	std::wcout << U("Hit Enter to close the listener.");
	std::getline(std::cin, line);
	// ����� ����������������� ����� �� getline main �������������, � ��������� � ��� server
	// �������� ���� ����������

}

