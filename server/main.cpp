#include "server.hpp"

using namespace std;
using namespace boost::asio;

int main()
{
	io_service service;
	ip::tcp::endpoint ep(ip::tcp::v4(), 19001);
	ip::tcp::acceptor acc(service, ep);

	cout << "Listening TCP-port " << ep.port() << endl;

	while (true)
	{
		socket_ptr new_client(new ip::tcp::socket(service));

		acc.accept(*new_client);

		string ip_string = new_client.get()->remote_endpoint().address().to_string();
		unsigned short port = new_client.get()->remote_endpoint().port();

		cout << "New client connected " << ip_string << ":" << port << endl;
		boost::thread(boost::bind(client_session, new_client));
	}

}
