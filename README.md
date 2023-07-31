# OrderBook
Order Book Programming Exercise in Cpp

cd ./ws

g++ -std=c++11  -I ./boost_1_82_0 server.cpp -o server && ./server



g++ -std=c++11  -I ./boost_1_82_0 client.cpp -o client && ./client


g++ -std=c++11  -I ./boost_1_82_0 udp_client.cpp -o udp_client && ./udp_client

g++ -std=c++11  -I ./boost_1_82_0 udp_server.cpp -o udp_server && ./udp_server

g++ -std=c++11  -I ./boost_1_82_0 parser.cpp csv_parser.cpp  -o parser && ./parser


cat ../client/input.csv | netcat -u 127.0.0.1 9002

