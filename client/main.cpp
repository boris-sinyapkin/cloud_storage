#include "client.hpp"

typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;

io_service service;
ip::tcp::socket sock(service);


int main()
{
	while (true)
	{
		try
		{
			int port;
			string command_line;
			string ip_string;

			cout << "Enter server address (ip:port): ";
			getline(cin, command_line);

			if (parse_servaddr(command_line, ip_string, &port) > 0)
			{
				cout << "	Connection..." << endl;
				ip::tcp::endpoint server(ip::address::from_string(ip_string), port);
				sock.connect(server);
				break;
			}
			else cout << "	Invalid address. Please, try again." << endl;
		}
		catch (boost::system::system_error &error)
		{
			cout << "	" << error.what() << endl;
			sock.close();
		}

	}

	cout << "	connect: Connection successful"<< endl;

	uint256_t login_hash;
	uint256_t auth_hash = authentification(sock, &login_hash);

	while (true)
	{
		int error = get_command(sock, auth_hash, login_hash);

		if (error < 0)
		{
			cout << "	Client terminating..." << endl;
			sleep(1);
			break;
		}
	}


	sock.close();


	return 0;
}
