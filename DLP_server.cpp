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

				// только данный класс, async_iostream, позволяет работать как с привычными вам std-потоками,
				// но асинхронно, совместно с в общем асинхронном аппарате cpprestsdk
				Concurrency::streams::async_istream<char>
					async_inout_stream2(intermediate_async_stream2.streambuf());

				async_inout_stream2 >> second_name; // "Иванов"

				/* пример чтения оставшегося содержимого файла в буфер и последующей работы с ним через потоки,
				первоисточник: cpprestsdk/Release/tests/functional/streams/stdstream_tests.cpp*/

				concurrency::streams::stringstreambuf tmp_stream_buffer; // асинхронный буфер для работы с содержимым файла 

				fileIStream.read_to_end(tmp_stream_buffer).get(); // считывание содержимого файла в буфер

				// чтобы работать с async_iostream (см. ниже) нужен данный промежуточный стрим
				auto intermediate_async_stream =
					Concurrency::streams::stringstream::
					open_istream(tmp_stream_buffer.collection()); 

				// только данный класс, async_iostream, позволяет работать как с привычными вам std-потоками,
				// но асинхронно, совместно с в общем асинхронном аппарате cpprestsdk
				Concurrency::streams::async_istream<char>
					async_inout_stream(intermediate_async_stream.streambuf());

				async_inout_stream >> first_name >> patronymic; // "Иван" "Иванович"

				fileIStream.close();

				/* вывод в файл (по примерам с https://casablanca.codeplex.com/wikipage?title=Asynchronous%20Streams,
				https://github.com/Microsoft/cpprestsdk/wiki/Getting-Started-Tutorial) */

				concurrency::streams::ostream fileOStream =
					concurrency::streams::file_stream<uint8_t>::
					open_ostream(U("DataBase.txt")).get();

				// связать файловый поток с потоком ввода/вывода
				Concurrency::streams::async_ostream<char>
					async_inout_stream3(fileOStream.streambuf());

				async_inout_stream3 << second_name << ' ' << first_name << ' ' << patronymic << "\r\n"; // запись данных, считанных из изначального файла
				async_inout_stream3 << "Петров" << ' ' << "Пётр" << ' ' << "Петрович" << "\r\n"; // запись новой строчки

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
		//concurrency::streams::basic_istream<char> async_input_stream();// async_string_buffer.create_istream());// промежуточный асинхронный поток ввода
		//std::basic_istream<char> standard_istream;// стандартный поток ввода для совместимости с вашим приложением
		//concurrency::streams::basic_ostream<char> async_output_stream;// промежуточный асинхронный поток вывода
		//std::basic_ostream<char> standard_ostream;// стандартный поток вывода для совместимости с вашим приложением

		void handle_get(web::http::http_request message) // http_request приходит от клиента
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
	
			/*std::wcout << L"Получен запрос GET" << std::endl;
			message.reply(status_codes::OK)
				.then([](pplx::task<void> t) {
				std::wcout << L"Отправлен ответ клиенту" << std::endl;
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
					[this, message](json::value jsonValue) // список захвата!!!
					{
						
						//std::wstring tmp = jsonValue.serialize();
						//std::wcout << jsonValue.serialize() << std::endl;// выводит json-запись в строке, не разбирая её
						std::wcout << jsonValue.at(U("password")).serialize() << std::endl;
						std::wcout << jsonValue.at(U("login")).serialize() << std::endl;
						std::wcout << jsonValue.at(U("captcha")).serialize() << std::endl;
						
						std::string line = async_string_buffer.collection();//передать содержимое всего файла (через буфер) в строку

						std::wstring wline = std::wstring(async_string_buffer.collection().begin(),
							async_string_buffer.collection().end()); // преобразование в 16-битную строку

						json::value replyData; // объект json-записи, затем - заполняется переменными
						replyData[U("string")] = json::value::string(wline);


						http_response response;
						response.set_reason_phrase(U("Access granted"));
						response.set_status_code(status_codes::OK);
						response.set_body(replyData);

						message.reply(response).wait();// запуск потока (цепочки task по отправке response обратно клиенту)
						
						
					});
				}
				catch (http_exception const & e)
				{
					std::wcout << e.what() << std::endl;
				}
				// если данного пользователя можно зарегистрировать в системе - регистрация
				// если нет - сообщение об отклонённой регистрации
			}
			else if (message.request_uri() == U("/dlpapi/auth"))
			{
				std::wcout << L"Authentification request" << std::endl;
				// если пользователь зарегистрирован с данной парой логин-пароль - выдать ему токен
				// если в данная пара логин-пароль отсутствует в системе - 
			}
			else if (message.request_uri() == U("/dlpapi/access"))
			{
				std::wcout << L"Database request access" << std::endl;
				// если пользователь аутентифицирован - выдать ему запрошенные данные
				// если пользоваель не известен - 

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
			// запись изменений
			//fileStream.close(); // закрытие файла

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

