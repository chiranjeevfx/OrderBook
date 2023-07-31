# OrderBook
Order Book Programming Exercise in Cpp


# run the following commands to run the code

# To run the udp server
g++ -std=c++11 udp_server.cpp -o udp_server && ./udp_server

# To run the udp client
g++ -std=c++11 udp_client.cpp -o udp_client && ./udp_client

# To run the code with input from the file
cat ./input.csv | netcat -u 127.0.0.1 9003

# To run in docker container
docker build -t order_book_image .
docker run -d -p 8080:8080/udp order_book_image
docker logs <CONTAINER ID>


#  Find the output in the kraken/output.csv file.


. This is a simple order book implementation in C++.
. It is a single threaded code, it will not work while multiple users are trying to execute the order into the market. 
We need to add process synchronization techniques to address this issue and scaling up the entire system.
. It is not taking time into the consideration while placing the order(Need to dig a bit deeper to resolve this issue).
. Space and Run time complexity of the code can be optimized.
. We can reduce number of lines.
. Time Complexity Analysis:
  1.  Deletion: O(n) --> Can be O(1) if we use double linked list instead of single linked iterator.
  2. Modification: O(n) --> Can be O(1) if we use double linked list instead of single linked iterator.
  3. Adding New Order: O(1)
  n: Total Number of Orders already working into the market
4. We need to structure the code to make it more readable and maintainable.
5. We need to add more test cases to make it more robust.
6. We need to add more comments to make it more readable.
7. We need to dockerize the code.
8. 
