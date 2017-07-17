#include <asio_pq/get_result.hpp>

#include <asio_pq/connect.hpp>
#include <asio_pq/connection.hpp>
#include <asio_pq/result.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/system/system_error.hpp>
#include <libpq-fe.h>
#include <utility>
#include <vector>
#include <catch.hpp>

#include "config.hpp"

namespace asio_pq {
namespace tests {
namespace {

static bool is_ok (const result & res) noexcept {
	switch (PQresultStatus(res)) {
	case PGRES_COMMAND_OK:
	case PGRES_TUPLES_OK:
		return true;
	default:
		break;
	}
	return false;
}

SCENARIO("PGresult objects may be acquired asynchronously via async_get_result", "[asio_pq][async_get_result]") {
	GIVEN("A boost::asio::io_service and an asio_pq::connection which manages a connection handle") {
		boost::asio::io_service ios;
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
		connection conn(keywords, values, false);
		REQUIRE(conn);
		auto future = async_connect(ios, conn, boost::asio::use_future);
		ios.run();
		ios.reset();
		future.get();
		REQUIRE(PQsetnonblocking(conn, 1) == 0);
		WHEN("PQsendQuery is used to submit command(s) to the database and the result(s) thereof are obtained by async_get_result") {
			REQUIRE(PQsendQuery(
				conn,
				"DROP TABLE IF EXISTS \"async_get_result_test\";"
				"CREATE TABLE \"async_get_result_test\" (\"num\" INT);"
				"INSERT INTO \"async_get_result_test\" VALUES (1), (2);"
				"SELECT * FROM \"async_get_result_test\";"
			) == 1);
			std::vector<result> rs;
			result r;
			for (;;) {
				bool invoked = false;
				async_get_result(ios, conn, [&] (auto ec, auto res) {
					invoked = true;
					if (ec) {
						INFO("boost::system::error_code::message: " << ec.message());
						INFO("PQerrorMessage: " << PQerrorMessage(conn));
						throw boost::system::system_error(ec);
					}
					r = std::move(res);
				});
				ios.run();
				REQUIRE(invoked);
				if (!r) break;
				bool ok = is_ok(r);
				if (!ok) INFO("PQresultErrorMessage: " << PQresultErrorMessage(r));
				REQUIRE(ok);
				ios.reset();
				rs.push_back(std::move(r));
			}
			THEN("The correct result(s) are retrieved") {
				REQUIRE(rs.size() == 4);
				CHECK(PQntuples(rs[0]) == 0);
				CHECK(PQntuples(rs[1]) == 0);
				CHECK(PQntuples(rs[2]) == 0);
				CHECK(PQntuples(rs[3]) == 2);
			}
		}
		WHEN("PQsendQuery is used to submit an invalid command to the database and the result is obtained by async_get_result") {
			REQUIRE(PQsendQuery(
				conn,
				"DELETE TABLE IF EXISTS \"async_get_result_test\";"
			) == 1);
			result res;
			bool invoked = false;
			auto f = [&] (auto ec, auto r) {
				invoked = true;
				if (ec) {
					INFO("boost::system::error_code::message: " << ec.message());
					INFO("PQerrorMessage: " << PQerrorMessage(conn));
					throw boost::system::system_error(ec);
				}
				res = std::move(r);
			};
			async_get_result(conn, f);
			ios.run();
			ios.reset();
			REQUIRE(invoked);
			THEN("A result is returned") {
				REQUIRE(res);
				AND_THEN("A failure is reported") {
					CHECK_FALSE(is_ok(res));
				}
			}
			AND_WHEN("The next result is obtained by async_get_result") {
				invoked = false;
				async_get_result(conn, f);
				ios.run();
				REQUIRE(invoked);
				THEN("A falsey result is returned") {
					CHECK_FALSE(res);
				}
			}
		}
	}
}

}
}
}
