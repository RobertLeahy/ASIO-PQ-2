add_executable(asio_pq_tests
	main.cpp
)
target_link_libraries(asio_pq_tests
	asio_pq
	Catch
)
add_test(NAME asio_pq COMMAND asio_pq_tests)
