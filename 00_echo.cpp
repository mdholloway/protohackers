#include <boost/asio.hpp>
#include <iostream>
#include <vector>

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 5000));

        std::cout << "Server listening on port 5000\n";

        while (true) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            std::vector<char> buffer(1024);
            boost::system::error_code error;

            while (true) {
                size_t length = socket.read_some(boost::asio::buffer(buffer), error);

                if (error == boost::asio::error::eof) {
                    break;
                } else if (error) {
                    throw boost::system::system_error(error);
                }

                boost::asio::write(socket, boost::asio::buffer(buffer, length));
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

