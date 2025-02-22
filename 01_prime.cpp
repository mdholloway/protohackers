#include <boost/asio.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <iostream>
#include <string>
#include <memory>
#include <cmath>

using boost::asio::ip::tcp;
namespace json = boost::json;

class Session : public std::enable_shared_from_this<Session> {
    tcp::socket socket_;
    std::string accumulated_data_;
    char data_[1024];

public:
    Session(boost::asio::io_context& io_context)
        : socket_(io_context) {}

    tcp::socket& socket() { return socket_; }

    void start() {
        do_read();
    }

private:
    bool is_prime(int64_t n) {
        if (n <= 1) return false;
        if (n <= 3) return true;
        if (n % 2 == 0 || n % 3 == 0) return false;

        for (int64_t i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0) return false;
        }
        return true;
    }

    bool process_line(const std::string& line, std::string& response) {
        try {
	    boost::system::error_code ec;
            json::value jv = json::parse(line, ec);
            if (ec) {
                response = "malformed\n";
                return false;
            }

            if (!jv.is_object()) {
                response = "malformed\n";
                return false;
            }

            json::object const& obj = jv.as_object();
            if (!obj.contains("method") || !obj.contains("number")) {
                response = "malformed\n";
                return false;
            }

            if (!obj.at("method").is_string() || obj.at("method").as_string() != "isPrime") {
                response = "malformed\n";
                return false;
            }

            auto number = obj.at("number");
            if (!number.is_number()) {
                response = "malformed\n";
                return false;
            }

            double num_double = json::value_to<double>(number);
            if (num_double != std::floor(num_double) || 
                num_double > 9223372036854775807.0 || 
                num_double < -9223372036854775808.0) {
                response = "{\"method\":\"isPrime\",\"prime\":false}\n";
            } else {
                int64_t num = static_cast<int64_t>(num_double);
                response = "{\"method\":\"isPrime\",\"prime\":" + 
                          std::string(is_prime(num) ? "true" : "false") + "}\n";
            }
            return true;
        } catch (...) {
            response = "malformed\n";
            return false;
        }
    }

    void do_read() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    accumulated_data_.append(data_, length);

                    size_t pos;
                    while ((pos = accumulated_data_.find('\n')) != std::string::npos) {
                        std::string line = accumulated_data_.substr(0, pos);
                        accumulated_data_.erase(0, pos + 1);

                        std::string response;
                        if (!process_line(line, response)) {
                            do_write(response);
                            socket_.close();
                            return;
                        }
                        do_write(response);
                    }
                    do_read();
                }
            });
    }

    void do_write(const std::string& response) {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_,
            boost::asio::buffer(response),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (ec) {
                    socket_.close();
                }
            });
    }
};

class Server {
    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;

public:
    Server(boost::asio::io_context& io_context)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), 5000)),
          io_context_(io_context) {
        do_accept();
    }

private:
    void do_accept() {
        auto session = std::make_shared<Session>(io_context_);
        acceptor_.async_accept(
            session->socket(),
            [this, session](boost::system::error_code ec) {
                if (!ec) {
                    session->start();
                }
                do_accept();
            });
    }
};

int main() {
    try {
        boost::asio::io_context io_context;
        Server server(io_context);

        std::cout << "Prime server listening on port 5000\n";
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

