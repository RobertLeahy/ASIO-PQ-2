#include <asio_pq/cancel.hpp>

#include <asio_pq/connect.hpp>
#include <asio_pq/connection.hpp>
#include <asio_pq/get_result.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/system/error_code.hpp>
#include <libpq-fe.h>
#include <cassert>
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
const char * values [] = {
	ASIO_PQ_TEST_HOST,
	ASIO_PQ_TEST_PORT,
	ASIO_PQ_TEST_USER,
	ASIO_PQ_TEST_PASSWORD,
	ASIO_PQ_TEST_DBNAME,
	nullptr
};

SCENARIO("Asynchronous connections may be cancelled", "[asio_pq][cancel][async_connect]") {
	GIVEN("A boost::asio::io_service and a connection handle") {
		boost::asio::io_service ios;
		connection conn(ios, keywords, values, false);
		WHEN("async_connect is invoked") {
			boost::system::error_code ec;
			bool invoked = false;
			async_connect(conn, [&] (auto e) noexcept {
				assert(!invoked);
				invoked = true;
				ec = e;
			});
			AND_WHEN("cancel is invoked") {
				cancel(conn);
				AND_WHEN("boost::asio::io_service::run is invoked") {
					ios.run();
					THEN("The connection attempt ends with boost::asio::error::operation_aborted") {
						CHECK(invoked);
						CHECK(ec);
						CHECK(ec == make_error_code(boost::asio::error::operation_aborted));
					}
				}
			}
		}
	}
}

SCENARIO("Asynchronous retrieval of results may be cancelled", "[asio_pq][cancel][async_get_result]") {
	GIVEN("A boost::asio::io_service and connected connection handle") {
		boost::asio::io_service ios;
		connection conn(ios, keywords, values, false);
		auto future = async_connect(conn, boost::asio::use_future);
		ios.run();
		future.get();
		ios.reset();
		WHEN("async_get_result is invoked") {
			REQUIRE(PQsendQuery(conn, "DROP TABLE IF EXISTS \"cancel_test\";") == 1);
			boost::system::error_code ec;
			bool invoked = false;
			async_get_result(conn, [&] (auto e, auto) noexcept {
				assert(!invoked);
				invoked = true;
				ec = e;
			});
			AND_WHEN("cancel is invoked") {
				cancel(conn);
				AND_WHEN("boost::asio::io_service::run is invoked") {
					ios.run();
					THEN("The attempt to retrieve results ends with boost::asio::error::operation_aborted") {
						CHECK(invoked);
						CHECK(ec);
						CHECK(ec == make_error_code(boost::asio::error::operation_aborted));
					}
				}
			}
		}
	}
}

}
}
}
