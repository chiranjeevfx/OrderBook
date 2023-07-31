//
// Created by Chiranjeev Kumar on 7/30/23.
//


#include <iostream>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <thread>
#include <list>
#include <algorithm>
using namespace std;

using boost::asio::ip::udp;



enum class OrderType { NEW, CANCEL };
enum class Side { Buy, Sell };

struct Order {
    OrderType type;
    int user;
    int userOrderId;
    string symbol;
    int price;
    int qty;
    char side;

    // Define order criteria
    bool operator<(const Order &rhs) const {
        return price < rhs.price; // orders with higher priority value come first
    }

    // Define the equality operator.
    bool operator==(const Order& other) const {
        return userOrderId == other.userOrderId;
    }
};

struct Trade {
    int userIdBuy;
    int userOrderIdBuy;
    int userIdSell;
    int userOrderIdSell;
    int price;
    int qty;
};

struct TopOfBook {
    int bidPrice;
    int bidQty;
    int askPrice;
    int askQty;

    void reset() {
        bidPrice = 0;
        bidQty = 0;
        askPrice = 0;
        askQty = 0;
    }
};

struct PriceLevel {
    int price;
    int totalVolume;
    list<Order> orders; // Need DLL implementation here
    void reset() {
        price = 0;
        totalVolume = 0;
        orders.clear();
    }
};

// Custom comparator
struct greater {
    template<class T>
    bool operator()(T const &a, T const &b) const { return a > b; }
};


struct BuyBook {
    map<int, PriceLevel, std::greater<int> > limitMap; // price -> PriceLevel
};

struct SellBook {
    map<int, PriceLevel> limitMap; // price -> PriceLevel
};

struct OrderBook {
    map<string, BuyBook> buyBook;
    map<string, SellBook> sellBook;
    PriceLevel bestBid;
    PriceLevel bestAsk;
    map<int, Order> orderMap; // orderId -> order
    vector<Trade> trades;
//    deque<Trade> trades;
    mutex m;
    TopOfBook tob;

    void flush() {
        std::lock_guard<std::mutex> lock(m); // Lock the mutex before modifying the data
        buyBook.clear();
        sellBook.clear();
        // Reset bestBid and bestAsk as needed
        bestBid.reset();
        bestAsk.reset();

        orderMap.clear();
        trades.clear();
        // Reset tob as needed
        tob.reset();
    } // Lock is automatically released here, when lock goes out of scope.
};

TopOfBook tob;


void publishTopOfBuyBook(const string& symbol, const TopOfBook& tob, OrderBook &orderBook, ofstream &outputFile) {
    if (orderBook.tob.bidQty > 0) {
        outputFile << "B,B," << orderBook.tob.bidPrice << "," << orderBook.tob.bidQty << endl;
        cout << "B,B," << orderBook.tob.bidPrice << "," << orderBook.tob.bidQty << endl;
    } else {
        outputFile << "B,B,-,-" << endl;
        cout << "B,B,-,-" << endl;
    }
}

void publishTopOfSellBook(const string& symbol, const TopOfBook& tob, OrderBook &orderBook, ofstream &outputFile) {
    if (orderBook.tob.askQty > 0) {
        outputFile << "B,S," << orderBook.tob.askPrice << "," << orderBook.tob.askQty << endl;
        cout << "B,S," << orderBook.tob.askPrice << "," << orderBook.tob.askQty << endl;
    } else {
        outputFile << "B,S,-,-" << endl;
        cout << "B,S,-,-" << endl;
    }
}

void updateTopOfBook(const string& symbol, OrderBook &orderBook, ofstream &outputFile) {

    bool isBuyChanged = false;
    bool isSellChanged = false;
    BuyBook &buyBook = orderBook.buyBook[symbol];
    SellBook &sellBook = orderBook.sellBook[symbol];
    if( !buyBook.limitMap.empty()) {
        // Get an iterator pointing to the first element
        auto it = buyBook.limitMap.begin();
        int price = it->first;
        PriceLevel &priceLevel = it->second;
        if (price != orderBook.bestBid.price || priceLevel.totalVolume != orderBook.bestBid.totalVolume) {
            orderBook.bestBid.price = price;
            orderBook.bestBid.totalVolume = priceLevel.totalVolume;
        }

        // update orderBook.tob
        if(price != orderBook.tob.bidPrice || priceLevel.totalVolume != orderBook.tob.bidQty) {
            orderBook.tob.bidPrice = price;
            orderBook.tob.bidQty = priceLevel.totalVolume;
            isBuyChanged = true;
        }
    } else {
        orderBook.bestBid.price = 0;
        orderBook.bestBid.totalVolume = 0;
    }

    if( !sellBook.limitMap.empty()) {
        // Get an iterator pointing to the first element
        auto it = sellBook.limitMap.begin();
        int price = it->first;
        PriceLevel &priceLevel = it->second;
        if (price != orderBook.bestAsk.price || priceLevel.totalVolume != orderBook.bestAsk.totalVolume) {
            orderBook.bestAsk.price = price;
            orderBook.bestAsk.totalVolume = priceLevel.totalVolume;
        }
        // update orderBook.tob
        if(price != orderBook.tob.askPrice || priceLevel.totalVolume != orderBook.tob.askQty) {
            orderBook.tob.askPrice = price;
            orderBook.tob.askQty = priceLevel.totalVolume;
            isSellChanged = true;
        }
    } else {
        orderBook.bestAsk.price = 0;
        orderBook.bestAsk.totalVolume = 0;
        if(orderBook.tob.askPrice != 0) {
            isSellChanged = true;
            orderBook.tob.askPrice = 0;
            orderBook.tob.askQty = 0;
        }
    }

    if(isSellChanged) {
        publishTopOfSellBook(symbol, orderBook.tob, orderBook, outputFile);
    }
    if(isBuyChanged) {
        publishTopOfBuyBook(symbol, orderBook.tob, orderBook, outputFile);
    }
}

void publishOrderAcknowledgement(int user, int userOrderId, ofstream &outputFile) {
    outputFile << "A," << user << "," << userOrderId << endl;
    cout << "A," << user << "," << userOrderId << endl;
}

void publishCancelAcknowledgement(int user, int userOrderId, ofstream &outputFile) {
    outputFile << "C," << user << "," << userOrderId << endl;
    cout << "C," << user << "," << userOrderId << endl;
}

void publishTrade(const Trade& trade, ofstream &outputFile) {
    outputFile << "T," << trade.userIdBuy << "," << trade.userOrderIdBuy << ","
         << trade.userIdSell << "," << trade.userOrderIdSell << ","
         << trade.price << "," << trade.qty << endl;
    cout << "T," << trade.userIdBuy << "," << trade.userOrderIdBuy << ","
         << trade.userIdSell << "," << trade.userOrderIdSell << ","
         << trade.price << "," << trade.qty << endl;
}

void handleNewOrder(const Order& order, OrderBook &orderBook, ofstream &outputFile) {
    orderBook.orderMap[order.userOrderId] = order;
    if (order.side == 'B') {
        BuyBook &buyBook = orderBook.buyBook[order.symbol];
        PriceLevel &priceLevel = buyBook.limitMap[order.price];
        priceLevel.totalVolume += order.qty;
        priceLevel.orders.push_back(order);
        if (order.price > orderBook.bestBid.price) {
            orderBook.bestBid.price = order.price;
            orderBook.bestBid.totalVolume = order.qty;
            orderBook.bestBid.orders.push_back(order);
        } else if (order.price == orderBook.bestBid.price) {
            orderBook.bestBid.totalVolume += order.qty;
            orderBook.bestBid.orders.push_back(order);
        }

    } else if (order.side == 'S') {
        SellBook &sellBook = orderBook.sellBook[order.symbol];
        PriceLevel &priceLevel = sellBook.limitMap[order.price];
        priceLevel.totalVolume += order.qty;
        priceLevel.orders.push_back(order);
        if (order.price < orderBook.bestAsk.price) {
            orderBook.bestAsk.price = order.price;
            orderBook.bestAsk.totalVolume = order.qty;
            orderBook.bestAsk.orders.push_back(order);
        } else if (order.price == orderBook.bestAsk.price) {
            orderBook.bestAsk.totalVolume += order.qty;
            orderBook.bestAsk.orders.push_back(order);
        }
    }
}

void deleteFromPriceLevel(const Order& order, PriceLevel& priceLevel) {
    priceLevel.totalVolume -= order.qty;
    // Find the element to delete.
    auto it = std::find(priceLevel.orders.begin(), priceLevel.orders.end(), order);

    // Ensure that the element was found before trying to delete.
    if (it != priceLevel.orders.end()) {
        priceLevel.orders.erase(it);
    }
}


void handleCancelOrder(const Order& order, OrderBook &orderBook, ofstream &outputFile) {
    Order &orderToDelete = orderBook.orderMap[order.userOrderId];
    orderBook.orderMap.erase(order.userOrderId);
    if (orderToDelete.side == 'B') {
        BuyBook &buyBook = orderBook.buyBook[orderToDelete.symbol];
        PriceLevel &priceLevel = orderBook.buyBook[orderToDelete.symbol].limitMap[orderToDelete.price];
        deleteFromPriceLevel(orderToDelete, priceLevel);
        if(priceLevel.totalVolume == 0) {
            orderBook.buyBook[orderToDelete.symbol].limitMap.erase(orderToDelete.price);
        }
        if(orderToDelete.price == orderBook.bestBid.price) {
            // Get an iterator pointing to the first element
            auto it = buyBook.limitMap.begin();

            if(priceLevel.totalVolume == 0) {
                orderBook.bestBid = it->second;
                publishTopOfBuyBook(orderToDelete.symbol, tob, orderBook, outputFile);
            }
        }

    } else if (orderToDelete.side == 'S') {
        SellBook &sellBook = orderBook.sellBook[orderToDelete.symbol];
        PriceLevel &priceLevel = orderBook.sellBook[orderToDelete.symbol].limitMap[orderToDelete.price];
        deleteFromPriceLevel(orderToDelete, priceLevel);
        if(priceLevel.totalVolume == 0) {
            orderBook.sellBook[orderToDelete.symbol].limitMap.erase(orderToDelete.price);
        }
        if(orderToDelete.price == orderBook.bestAsk.price) {
            // Get an iterator pointing to the first element
            auto it = sellBook.limitMap.begin();

            if(priceLevel.totalVolume == 0) {
                orderBook.bestAsk = it->second;
                publishTopOfSellBook(orderToDelete.symbol, tob, orderBook, outputFile   );
            }
        }
    }
}

string getTradeMessage(Trade &trade) {
    string tradeMessage = "T," + to_string(trade.userIdBuy) + "," + to_string(trade.userOrderIdBuy) + ","
                          + to_string(trade.userIdSell) + "," + to_string(trade.userOrderIdSell) + ","
                          + to_string(trade.price) + "," + to_string(trade.qty);
    return tradeMessage;
}

void handleMatch(const Order& order, vector<Trade>& trades, OrderBook &orderBook, ofstream &outputFile) {
    // Matching logic
//    cout<<"Matching logic"<<endl;
    BuyBook buyBook = orderBook.buyBook[order.symbol];
    SellBook sellBook = orderBook.sellBook[order.symbol];
    if (buyBook.limitMap.count(order.price) && sellBook.limitMap.count(order.price)) {
        PriceLevel &buyPriceLevel = buyBook.limitMap[order.price];
        PriceLevel &sellPriceLevel = sellBook.limitMap[order.price];
        while (!buyPriceLevel.orders.empty() && !sellPriceLevel.orders.empty()) {
            Order &buyOrder = buyPriceLevel.orders.front();
            Order &sellOrder = sellPriceLevel.orders.front();
            int tradeQty = min(buyOrder.qty, sellOrder.qty);

            Trade trade = {
                    buyOrder.user,
                    buyOrder.userOrderId,
                    sellOrder.user,
                    sellOrder.userOrderId,
                    sellOrder.price,
                    tradeQty
            };
            outputFile << getTradeMessage(trade) << endl;
            cout<< getTradeMessage(trade) << endl;

            trades.push_back(trade);

            buyOrder.qty -= tradeQty;
            sellOrder.qty -= tradeQty;

            if (buyOrder.qty == 0) {
                buyPriceLevel.orders.pop_front();
            }
            if (sellOrder.qty == 0) {
                sellPriceLevel.orders.pop_front();
            }
        }
        if (buyPriceLevel.orders.empty()) {
            buyBook.limitMap.erase(order.price);
        }
        if (sellPriceLevel.orders.empty()) {
            sellBook.limitMap.erase(order.price);
        }
    }
}

void processOrder(const Order& order, vector<Trade>& trades,
                  mutex& orderBooksMutex, OrderBook &orderBook, ofstream &outputFile) {
    lock_guard<mutex> lock(orderBooksMutex);
    if (order.type == OrderType::NEW) {
        handleNewOrder(order, orderBook, outputFile);
    } else if (order.type == OrderType::CANCEL) {
        handleCancelOrder(order, orderBook, outputFile);
    }
    handleMatch(order, trades, orderBook, outputFile);
}

Order parseOrder(const std::vector<std::string>& inputLine) {
    if (inputLine.size() != 7) {
        // Error handling for invalid input
        throw std::runtime_error("Invalid input line");
    }

    Order order;
    order.type = OrderType::NEW,
            order.user = std::stoi(inputLine[1]);
    order.userOrderId = std::stoi(inputLine[6]);
    order.symbol = inputLine[2];
    order.price = std::stoi(inputLine[3]);
    order.qty = std::stoi(inputLine[4]);
    order.side = inputLine[5][0];
    return order;
}

void processRequest(string line, vector<Trade> &trades, mutex& orderBooksMutex,
                    OrderBook &orderBook, ofstream &outputFile) {
//    outputFile << line << endl  ;
    if (line.empty() || line[0] == '#') {
        return;
    }

    vector<string> inputLine;
    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
    istringstream iss(line);
    string cell;
    while (getline(iss, cell, ',')) {
        inputLine.push_back(cell);
    }

    if (inputLine[0] == "N") {
        Order order = { OrderType::NEW, stoi(inputLine[1]), stoi(inputLine[6]),
                        inputLine[2], stoi(inputLine[3]), stoi(inputLine[4]),
                        inputLine[5][0]};
        processOrder(order, trades, orderBooksMutex, orderBook, outputFile);
        publishOrderAcknowledgement(order.user, order.userOrderId, outputFile);
        updateTopOfBook(inputLine[2], orderBook, outputFile);
        return;
    } else if (inputLine[0] == "C") {
        Order order = { OrderType::CANCEL, stoi(inputLine[1]), stoi(inputLine[2]),
                        "", 0, 0, ' '};
        processOrder(order, trades, orderBooksMutex, orderBook, outputFile);
        publishCancelAcknowledgement(order.user, order.userOrderId, outputFile);
        updateTopOfBook(inputLine[2], orderBook, outputFile);
    } else if (inputLine[0] == "F") {
        orderBook.flush();
        outputFile << "F" << endl;
        cout << "\n" << endl;
    }
}

int main() {
    boost::asio::io_context io_context;

    udp::socket socket(io_context, udp::endpoint(udp::v4(), 9003));
    OrderBook orderBook;

    vector<Trade> trades;
    mutex orderBooksMutex;
    string outputFileName = "output_file.csv";
    ofstream outputFile(outputFileName);

    while (true) {
        char data[1024];
        udp::endpoint remote_endpoint;

        boost::system::error_code error;
        size_t length = socket.receive_from(boost::asio::buffer(data), remote_endpoint, 0, error);

        if (error && error != boost::asio::error::message_size) {
            std::cerr << "Error receiving data: " << error.message() << std::endl;
            continue;
        }

//        std::cout << "Received message from " << remote_endpoint.address().to_string() << ":" << remote_endpoint.port() << std::endl;
//        std::cout << "Message: " << data << std::endl;
        processRequest(data, trades, orderBooksMutex, orderBook, outputFile);

        socket.send_to(boost::asio::buffer(data, length), remote_endpoint, 0, error);
        if (error) {
            std::cerr << "Error sending data: " << error.message() << std::endl;
        }
    }

    return 0;
}
