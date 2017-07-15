#include <asio_pq/connect.hpp>

#include <asio_pq/handle.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <utility>
#include <catch.hpp>

#include "config.hpp"

namespace asio_pq {
namespace tests {
namespace {

const char * keywords [] = {
	"host",
	"port",
	"user",
	"password",
	"dbname",
	nullptr
};

SCENARIO("async_connect may be used to asynchronously connect to a PostgreSQL server", "[asio_pq][async_connect]") {
	GIVEN("A boost::asio::io_service") {
		boost::asio::io_service ios;
		WHEN("A call to async_connect is made with a valid connection info string") {
			boost::system::error_code ec;
			bool invoked = false;
			asio_pq::handle handle;
			asio_pq::async_connect(
				ios,
				"host=" ASIO_PQ_TEST_HOST
				" port=" ASIO_PQ_TEST_PORT
				" user=" ASIO_PQ_TEST_USER
				" password=" ASIO_PQ_TEST_PASSWORD
				" dbname=" ASIO_PQ_TEST_DBNAME,
				[&] (boost::system::error_code inner, asio_pq::handle h) noexcept {
					invoked = true;
					ec = inner;
					handle = std::move(h);
				}
			);
			THEN("The operation does not complete") {
				CHECK_FALSE(invoked);
			}
			AND_WHEN("boost::asio::io_service::run is called") {
				ios.run();
				THEN("The operation completes") {
					REQUIRE(invoked);
					AND_THEN("The operation does not fail") {
						REQUIRE(handle);
						INFO("boost::system::error_code::message: " << ec.message());
						INFO("PQerrorMessage: " << PQerrorMessage(handle));
						CHECK_FALSE(ec);
					}
				}
			}
		}
		WHEN("A call to async_connect is made with valid parameters") {
			boost::system::error_code ec;
			bool invoked = false;
			asio_pq::handle handle;
			const char * values [] = {
				ASIO_PQ_TEST_HOST,
				ASIO_PQ_TEST_PORT,
				ASIO_PQ_TEST_USER,
				ASIO_PQ_TEST_PASSWORD,
				ASIO_PQ_TEST_DBNAME,
				nullptr
			};
			asio_pq::async_connect(
				ios,
				keywords,
				values,
				false,
				[&] (boost::system::error_code inner, asio_pq::handle h) noexcept {
					invoked = true;
					ec = inner;
					handle = std::move(h);
				}
			);
			THEN("The operation does not complete") {
				CHECK_FALSE(invoked);
			}
			AND_WHEN("boost::asio::io_service::run is called") {
				ios.run();
				THEN("The operation completes") {
					REQUIRE(invoked);
					AND_THEN("The operation does not fail") {
						REQUIRE(handle);
						INFO("boost::system::error_code::message: " << ec.message());
						INFO("PQerrorMessage: " << PQerrorMessage(handle));
						CHECK_FALSE(ec);
					}
				}
			}
		}
		WHEN("A call to async_connect is made with an invalid connection info string") {
			boost::system::error_code ec;
			bool invoked = false;
			asio_pq::handle handle;
			asio_pq::async_connect(
				ios,
				"username=asiopq",
				[&] (boost::system::error_code inner, asio_pq::handle h) noexcept {
					invoked = true;
					ec = inner;
					handle = std::move(h);
				}
			);
			THEN("The operation does not complete") {
				CHECK_FALSE(invoked);
			}
			AND_WHEN("boost::asio::io_service::run is called") {
				ios.run();
				THEN("The operation completes") {
					REQUIRE(invoked);
					AND_THEN("The operation fails") {
						REQUIRE(handle);
						INFO("boost::system::error_code::message: " << ec.message());
						INFO("PQerrorMessage: " << PQerrorMessage(handle));
						CHECK(ec);
					}
				}
			}
		}
		WHEN("A call to async_connect is made with invalid parameters") {
			boost::system::error_code ec;
			bool invoked = false;
			asio_pq::handle handle;
			const char * keywords [] = {"username", nullptr};
			const char * values [] = {"foo", nullptr};
			asio_pq::async_connect(
				ios,
				keywords,
				values,
				false,
				[&] (boost::system::error_code inner, asio_pq::handle h) noexcept {
					invoked = true;
					ec = inner;
					handle = std::move(h);
				}
			);
			THEN("The operation does not complete") {
				CHECK_FALSE(invoked);
			}
			AND_WHEN("boost::asio::io_service::run is called") {
				ios.run();
				THEN("The operation completes") {
					REQUIRE(invoked);
					AND_THEN("The operation fails") {
						REQUIRE(handle);
						INFO("boost::system::error_code::message: " << ec.message());
						INFO("PQerrorMessage: " << PQerrorMessage(handle));
						CHECK(ec);
					}
				}
			}
		}
		WHEN("A call to async_connect is made with a valid connection string containing invalid information") {
			boost::system::error_code ec;
			bool invoked = false;
			asio_pq::handle handle;
			asio_pq::async_connect(
				ios,
				"host=" ASIO_PQ_TEST_BAD_HOST
				" port=" ASIO_PQ_TEST_BAD_PORT
				" user=" ASIO_PQ_TEST_BAD_USER
				" password=" ASIO_PQ_TEST_BAD_PASSWORD
				" dbname=" ASIO_PQ_TEST_BAD_DBNAME,
				[&] (boost::system::error_code inner, asio_pq::handle h) noexcept {
					invoked = true;
					ec = inner;
					handle = std::move(h);
				}
			);
			THEN("The operation does not complete") {
				CHECK_FALSE(invoked);
			}
			AND_WHEN("boost::asio::io_service::run is called") {
				ios.run();
				THEN("The operation completes") {
					REQUIRE(invoked);
					AND_THEN("The operation fails") {
						REQUIRE(handle);
						INFO("boost::system::error_code::message: " << ec.message());
						INFO("PQerrorMessage: " << PQerrorMessage(handle));
						CHECK(ec);
					}
				}
			}
		}
		WHEN("A call to async_connect is made with invalid parameters") {
			boost::system::error_code ec;
			bool invoked = false;
			asio_pq::handle handle;
			const char * values [] = {
				ASIO_PQ_TEST_BAD_HOST,
				ASIO_PQ_TEST_BAD_PORT,
				ASIO_PQ_TEST_BAD_USER,
				ASIO_PQ_TEST_BAD_PASSWORD,
				ASIO_PQ_TEST_BAD_DBNAME,
				nullptr
			};
			asio_pq::async_connect(
				ios,
				keywords,
				values,
				false,
				[&] (boost::system::error_code inner, asio_pq::handle h) noexcept {
					invoked = true;
					ec = inner;
					handle = std::move(h);
				}
			);
			THEN("The operation does not complete") {
				CHECK_FALSE(invoked);
			}
			AND_WHEN("boost::asio::io_service::run is called") {
				ios.run();
				THEN("The operation completes") {
					REQUIRE(invoked);
					AND_THEN("The operation fails") {
						REQUIRE(handle);
						INFO("boost::system::error_code::message: " << ec.message());
						INFO("PQerrorMessage: " << PQerrorMessage(handle));
						CHECK(ec);
					}
				}
			}
		}
	}
}

}
}
}
