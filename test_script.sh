#!/bin/bash

# How to read this file: 
#
# This file is written so that the critical commands can be understood clearly. You will see that every thing that
# relates to formatting the test output (and cleaning up) is indented with four tabs. This is intentional to make
# sure that the commands that you may want to copy on your terminal when trying to debug something are clearly
# visible. 

set -e # Comment this line if you want to keep your test running even if something failed (this will prevent the test from failing).

					if [ -z "$var" ]; then export TERM=ansi; fi

					function show_output {
						tput bold; tput setaf 6; echo " [-] Showing server output"; tput sgr0;

						echo
						tput bold; tput setaf 6; echo "-------------------- Start of Server Output --------------------"; tput sgr0;
						cat torrent_samples/server_output.tmp
						tput bold; tput setaf 6; echo "-------------------- End of Server Output   --------------------"; tput sgr0;
						echo
					
						tput bold; tput setaf 6; echo " [-] Showing client output"; tput sgr0;

						echo
						tput bold; tput setaf 6; echo "-------------------- Start of Client Output --------------------"; tput sgr0;
						cat torrent_samples/client_output.tmp
						tput bold; tput setaf 6; echo "-------------------- End of Client Output   --------------------"; tput sgr0;
						echo
					}

					function finish_before_log {
						tput bold; tput setaf 1; echo "Execution failed: $BASH_COMMAND (the program crashed or returned a non-zero value; you may want to reproduce this failure outside of the test environment and debug your code)"; tput sgr0;
					
						show_output

						rm -f torrent_samples/client/test_file torrent_samples/client_output.tmp torrent_samples/server_output.tmp
	
						tput bold; tput setaf 4;
						echo "##################################################################"
						echo "TESTS RESULT: $RESULT"
						echo "##################################################################"
						tput sgr0;
	
						kill -9 "$SERVERPID" 2> /dev/null > /dev/null
					}

					function finish_log_ok {
						rm -f torrent_samples/client/test_file torrent_samples/client_output.tmp torrent_samples/server_output.tmp
	
						tput bold; tput setaf 4;
						echo "##################################################################"
						echo "TESTS RESULT: $RESULT"
						echo "##################################################################"
						tput sgr0;
					}

# We start with a failed test and if everything works, we change this at the end of the test.
RESULT="FAIL"

				tput bold; tput setaf 4; # This is just so that the output colored and readeable
				echo "##################################################################"
				echo "TRIVIAL TORRENT CLIENT (testing against reference server)"
				echo "##################################################################"
				tput sgr0; # Reset colors

				trap finish_before_log EXIT

# Ensure file is not already there!
rm -f torrent_samples/client/test_file

				tput bold; tput setaf 6; echo " [-] Starting Reference Server"; tput sgr0;

# Launch server
reference_binary/ttorrent -l 8080 torrent_samples/server/test_file_server.ttorrent > torrent_samples/server_output.tmp 2>&1 &
				SERVERPID=$!

				tput bold; tput setaf 6; echo " [-] Waiting one second for it to start"; tput sgr0;

				sleep 1

				tput bold; tput setaf 6; echo " [-] Starting Client"; tput sgr0;

# Launch client
bin/ttorrent torrent_samples/client/test_file.ttorrent > torrent_samples/client_output.tmp 2>&1

				trap finish_log_ok EXIT

				show_output

				tput bold; tput setaf 6; echo " [-] Killing server..."; tput sgr0;

				kill -9 "$SERVERPID" 2> /dev/null > /dev/null || { tput bold; tput setaf 1; echo "Server is dead but should be alive!"; tput sgr0; exit 1 ; }

				tput bold; tput setaf 6; echo " [-] Comparing downloaded file"; tput sgr0;

# Compare result
cmp torrent_samples/server/test_file_server torrent_samples/client/test_file || {
	tput bold; tput setaf 1; echo "ERROR: downloaded file does not match!"; tput sgr0; false;
}

				rm -f torrent_samples/client/test_file torrent_samples/client_output.tmp torrent_samples/server_output.tmp

				tput bold; tput setaf 4;
				echo "##################################################################"
				echo "TRIVIAL TORRENT SERVER (testing against reference client)"
				echo "##################################################################"
				tput sgr0; 

				trap finish_before_log EXIT

				tput bold; tput setaf 6; echo " [-] Starting Server"; tput sgr0;

# Launch server
bin/ttorrent -l 8080 torrent_samples/server/test_file_server.ttorrent > torrent_samples/server_output.tmp 2>&1 &
				SERVERPID=$!

				tput bold; tput setaf 6; echo " [-] Waiting one second for it to start"; tput sgr0;

				sleep 1

				tput bold; tput setaf 6; echo " [-] Starting Reference Client"; tput sgr0;

# Launch client
reference_binary/ttorrent torrent_samples/client/test_file.ttorrent > torrent_samples/client_output.tmp 2>&1

				trap finish_log_ok EXIT

				show_output

				tput bold; tput setaf 6; echo " [-] Killing server..."; tput sgr0;

				kill -9 "$SERVERPID" 2> /dev/null > /dev/null || { tput bold; tput setaf 1; echo "Server is dead but should be alive!"; tput sgr0; exit 1 ; }

				tput bold; tput setaf 6; echo " [-] Comparing downloaded file"; tput sgr0;

# Compare result
cmp torrent_samples/server/test_file_server torrent_samples/client/test_file || {
	tput bold; tput setaf 1; echo "ERROR: downloaded file does not match!"; tput sgr0; false;
}

				rm -f torrent_samples/client/test_file torrent_samples/client_output.tmp torrent_samples/server_output.tmp

# Test passed
RESULT="PASS"

