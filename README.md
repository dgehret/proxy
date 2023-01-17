I - Useage
	To use this application, run make and enter your commands in the following format:

	(if in the top directory)

		bin/myproxy listen_port forbidden_sites_file_path access_log_file_path

		wget http://www.example_domain.com/ -e use_proxy=yes -e http_proxy=127.0.0.1:listen_port



II - Internal Design
	The internal design is as follows:
		
		0) functions

		1) parse command line parameters

		2) initialize connection with localhost, listen on port from command line
			a) error checking

		3) initialize connection with web server at the address parsed from localhost request
			a) error checking

		4) form and send over request

		5) receive document from desired web server



III - Shortcomings
	
	* Initial client/proxy connection. Not quite sure why 'wget' is not allowing 
		for a proper connection with methods I have successfully implemented before.



IV - Testing


	bin/myproxy 40123 proxy_sites outfile

	wget http://www.google.com/ -e use_proxy=yes -e http_proxy=127.0.0.1:40123



	bin/myproxy 9080 proxy_sites outfile

	wget http://www.example.com/ -e use_proxy=yes -e http_proxy=127.0.0.3:9080



	bin/myproxy 42424 proxy_sites outfile

	wget http://www.facebook.com/ -e use_proxy=yes -e http_proxy=127.0.0.2:42424



	bin/myproxy 6900 proxy_sites outfile

	wget http://www.google.com/ -e use_proxy=yes -e http_proxy=127.0.0.1:6900



	bin/myproxy 42000 proxy_sites outfile

	wget http://www.google.com/ -e use_proxy=yes -e http_proxy=127.0.0.1:42000


