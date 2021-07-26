// DLP_server.cpp: определяет точку входа для консольного приложения.
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
			listener = http_listener(U("http://localhost:12345"));//TODO создать файл
			listener.support(methods::GET, // для запросов-GET
				std::bind(&DLPServer::handle_get, this, std::placeholders::_1));// обработчиком будет функция handle_get с 1 параметром
			listener.support(methods::POST,
				std::bind(&DLPServer::handle_post, this, std::placeholders::_1));
			listener.open().wait(); // запуск сервера
			
		}

		void example_file_operations()
		{
			try
			{
				//запускается отдельный поток по открытию файла, но благодаря get основной поток его ожидает
				concurrency::streams::istream fileIStream =
					concurrency::streams::file_stream<uint8_t>::
					open_istream(U("DataBase.txt")).get(); 

				std::string second_name("");
				std::string first_name("");
				std::string patronymic("");

				/* пример считывания из файла до разделителя, аналогично - чтение построчно через fileStream.read_line */
				
				Concurrency::streams::stringstreambuf tmp_stream_buffer2;

				fileIStream.read_to_delim(tmp_stream_buffer2, ' '/*символ-разделитель*/).get();
				
				// чтобы работать с async_iostream (см. ниже) нужен данный промежуточный стрим
				auto intermediate_async_stream2 =
					Concurrency::streams::stringstream::
					open_istream(tmp_stream_buffer2.collection()); 

				// òîëüêî äàííûé êëàññ, async_iostream, ïîçâîëÿåò ðàáîòàòü êàê ñ ïðèâû÷íûìè âàì std-ïîòîêàìè,
				// íî àñèíõðîííî, ñîâìåñòíî ñ â îáùåì àñèíõðîííîì àïïàðàòå cpprestsdk
				Concurrency::streams::async_istream<char>
					async_inout_stream2(intermediate_async_stream2.streambuf());

				async_inout_stream2 >> second_name; // "Èâàíîâ"

				/* ïðèìåð ÷òåíèÿ îñòàâøåãîñÿ ñîäåðæèìîãî ôàéëà â áóôåð è ïîñëåäóþùåé ðàáîòû ñ íèì ÷åðåç ïîòîêè,
				ïåðâîèñòî÷íèê: cpprestsdk/Release/tests/functional/streams/stdstream_tests.cpp*/

				concurrency::streams::stringstreambuf tmp_stream_buffer; // àñèíõðîííûé áóôåð äëÿ ðàáîòû ñ ñîäåðæèìûì ôàéëà 

				fileIStream.read_to_end(tmp_stream_buffer).get(); // ñ÷èòûâàíèå ñîäåðæèìîãî ôàéëà â áóôåð

				// ÷òîáû ðàáîòàòü ñ async_iostream (ñì. íèæå) íóæåí äàííûé ïðîìåæóòî÷íûé ñòðèì
				auto intermediate_async_stream =
					Concurrency::streams::stringstream::
					open_istream(tmp_stream_buffer.collection()); 

				// òîëüêî äàííûé êëàññ, async_iostream, ïîçâîëÿåò ðàáîòàòü êàê ñ ïðèâû÷íûìè âàì std-ïîòîêàìè,
				// íî àñèíõðîííî, ñîâìåñòíî ñ â îáùåì àñèíõðîííîì àïïàðàòå cpprestsdk
				Concurrency::streams::async_istream<char>
					async_inout_stream(intermediate_async_stream.streambuf());

				async_inout_stream >> first_name >> patronymic; // "Èâàí" "Èâàíîâè÷"

				fileIStream.close();

				/* âûâîä â ôàéë (ïî ïðèìåðàì ñ https://casablanca.codeplex.com/wikipage?title=Asynchronous%20Streams,
				https://github.com/Microsoft/cpprestsdk/wiki/Getting-Started-Tutorial) */

				concurrency::streams::ostream fileOStream =
					concurrency::streams::file_stream<uint8_t>::
					open_ostream(U("DataBase.txt")).get();

				// ñâÿçàòü ôàéëîâûé ïîòîê ñ ïîòîêîì ââîäà/âûâîäà
				Concurrency::streams::async_ostream<char>
					async_inout_stream3(fileOStream.streambuf());

				async_inout_stream3 << second_name << ' ' << first_name << ' ' << patronymic << "\r\n"; // çàïèñü äàííûõ, ñ÷èòàííûõ èç èçíà÷àëüíîãî ôàéëà
				async_inout_stream3 << "Ïåòðîâ" << ' ' << "Ï¸òð" << ' ' << "Ïåòðîâè÷" << "\r\n"; // çàïèñü íîâîé ñòðî÷êè

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
		//concurrency::streams::basic_istream<char> async_input_stream();// async_string_buffer.create_istream());// ïðîìåæóòî÷íûé àñèíõðîííûé ïîòîê ââîäà
		//std::basic_istream<char> standard_istream;// ñòàíäàðòíûé ïîòîê ââîäà äëÿ ñîâìåñòèìîñòè ñ âàøèì ïðèëîæåíèåì
		//concurrency::streams::basic_ostream<char> async_output_stream;// ïðîìåæóòî÷íûé àñèíõðîííûé ïîòîê âûâîäà
		//std::basic_ostream<char> standard_ostream;// ñòàíäàðòíûé ïîòîê âûâîäà äëÿ ñîâìåñòèìîñòè ñ âàøèì ïðèëîæåíèåì

		void handle_get(web::http::http_request message) // http_request ïðèõîäèò îò êëèåíòà
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
	
			/*std::wcout << L"Ïîëó÷åí çàïðîñ GET" << std::endl;
			message.reply(status_codes::OK)
				.then([](pplx::task<void> t) {
				std::wcout << L"Îòïðàâëåí îòâåò êëèåíòó" << std::endl;
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
					[this, message](json::value jsonValue) // ñïèñîê çàõâàòà!!!
					{
						
						//std::wstring tmp = jsonValue.serialize();
						//std::wcout << jsonValue.serialize() << std::endl;// âûâîäèò json-çàïèñü â ñòðîêå, íå ðàçáèðàÿ å¸
						std::wcout << jsonValue.at(U("password")).serialize() << std::endl;
						std::wcout << jsonValue.at(U("login")).serialize() << std::endl;
						std::wcout << jsonValue.at(U("captcha")).serialize() << std::endl;
						
						std::string line = async_string_buffer.collection();//ïåðåäàòü ñîäåðæèìîå âñåãî ôàéëà (÷åðåç áóôåð) â ñòðîêó

						std::wstring wline = std::wstring(async_string_buffer.collection().begin(),
							async_string_buffer.collection().end()); // ïðåîáðàçîâàíèå â 16-áèòíóþ ñòðîêó

						json::value replyData; // îáúåêò json-çàïèñè, çàòåì - çàïîëíÿåòñÿ ïåðåìåííûìè
						replyData[U("string")] = json::value::string(wline);


						http_response response;
						response.set_reason_phrase(U("Access granted"));
						response.set_status_code(status_codes::OK);
						response.set_body(replyData);

						message.reply(response).wait();// çàïóñê ïîòîêà (öåïî÷êè task ïî îòïðàâêå response îáðàòíî êëèåíòó)
						
						
					});
				}
				catch (http_exception const & e)
				{
					std::wcout << e.what() << std::endl;
				}
				// åñëè äàííîãî ïîëüçîâàòåëÿ ìîæíî çàðåãèñòðèðîâàòü â ñèñòåìå - ðåãèñòðàöèÿ
				// åñëè íåò - ñîîáùåíèå îá îòêëîí¸ííîé ðåãèñòðàöèè
			}
			else if (message.request_uri() == U("/dlpapi/auth"))
			{
				std::wcout << L"Authentification request" << std::endl;
				// åñëè ïîëüçîâàòåëü çàðåãèñòðèðîâàí ñ äàííîé ïàðîé ëîãèí-ïàðîëü - âûäàòü åìó òîêåí
				// åñëè â äàííàÿ ïàðà ëîãèí-ïàðîëü îòñóòñòâóåò â ñèñòåìå - 
			}
			else if (message.request_uri() == U("/dlpapi/access"))
			{
				std::wcout << L"Database request access" << std::endl;
				// åñëè ïîëüçîâàòåëü àóòåíòèôèöèðîâàí - âûäàòü åìó çàïðîøåííûå äàííûå
				// åñëè ïîëüçîâàåëü íå èçâåñòåí - 

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
			// çàïèñü èçìåíåíèé
			//fileStream.close(); // çàêðûòèå ôàéëà

		}
};


int main()
{
	setlocale(LC_ALL, "Russian");
	DLPServer server;
	
	std::string line;
	std::wcout << U("Hit Enter to close the listener.");
	std::getline(std::cin, line);
	// после пользовательского ввода на getline main заканчивается, и созданный в ней server
	// вызывает свой деструктор

}

