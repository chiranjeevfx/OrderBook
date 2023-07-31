//
// Created by Chiranjeev Kumar on 7/29/23.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <queue>
#include <thread>
#include <mutex>
#include <list>
#include <algorithm>
using namespace std;

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
    list<Order> orders; // DLL implementation
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
    deque<Trade> trades;
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

void publishTopOfBuyBook(const string& symbol, const TopOfBook& tob, OrderBook &orderBook) {
    if (orderBook.tob.bidQty > 0) {
        cout << "B,B," << orderBook.tob.bidPrice << "," << orderBook.tob.bidQty << endl;
    } else {
        cout << "B,B,-,-" << endl;
    }
}

void publishTopOfSellBook(const string& symbol, const TopOfBook& tob, OrderBook &orderBook) {
    if (orderBook.tob.askQty > 0) {
        cout << "B,S," << orderBook.tob.askPrice << "," << orderBook.tob.askQty << endl;
    } else {
        cout << "B,S,-,-" << endl;
    }
}

TopOfBook getTopOfBook(const string& symbol, map<string,priority_queue<Order> > & buy_order_books,
map<string, priority_queue<Order> > & sell_order_books, OrderBook &orderBook) {

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
//        if(orderBook.tob.askPrice != 0) {
//            isSellChanged = true;
//            orderBook.tob.askPrice = 0;
//            orderBook.tob.askQty = 0;
//        }
    }

    if(isSellChanged) {
        publishTopOfSellBook(symbol, orderBook.tob, orderBook);
    }
    if(isBuyChanged) {
        publishTopOfBuyBook(symbol, orderBook.tob, orderBook);
    }
    return tob;
}




void publishOrderAcknowledgement(int user, int userOrderId) {
    cout << "A," << user << "," << userOrderId << endl;
}

void publishCancelAcknowledgement(int user, int userOrderId) {
    cout << "C," << user << "," << userOrderId << endl;
}

void publishTopOfBook(const string& symbol, char side, int price, int totalQty) {
    cout << "B," << side << "," << price << "," << totalQty << endl;
}

void publishTrade(const Trade& trade) {
    cout << "T," << trade.userIdBuy << "," << trade.userOrderIdBuy << ","
         << trade.userIdSell << "," << trade.userOrderIdSell << ","
         << trade.price << "," << trade.qty << endl;
}

void handleNewOrder(const Order& order, map<string, priority_queue<Order> >& buy_order_books,
                  map<string, priority_queue<Order> >& sell_order_books, vector<Trade>& trades,
                  mutex& orderBooksMutex, OrderBook &orderBook) {
    orderBook.orderMap[order.userOrderId] = order;
    if (order.side == 'B') {
        buy_order_books[order.symbol].push(order); // delete it from here
        BuyBook &buyBook = orderBook.buyBook[order.symbol];
        PriceLevel &priceLevel = buyBook.limitMap[order.price];
        priceLevel.totalVolume += order.qty;
        priceLevel.orders.push_back(order);
        if (order.price > orderBook.bestBid.price) {
            orderBook.bestBid.price = order.price;
            orderBook.bestBid.totalVolume = order.qty;
            orderBook.bestBid.orders.push_back(order);
//            publishTopOfBook(order.symbol, 'B', order.price, order.qty);
        } else if (order.price == orderBook.bestBid.price) {
            orderBook.bestBid.totalVolume += order.qty;
            orderBook.bestBid.orders.push_back(order);
        }

    } else if (order.side == 'S') {
        sell_order_books[order.symbol].push(order); // delete it from here
        SellBook &sellBook = orderBook.sellBook[order.symbol];
        PriceLevel &priceLevel = sellBook.limitMap[order.price];
        priceLevel.totalVolume += order.qty;
        priceLevel.orders.push_back(order);
        if (order.price < orderBook.bestAsk.price) {
            orderBook.bestAsk.price = order.price;
            orderBook.bestAsk.totalVolume = order.qty;
            orderBook.bestAsk.orders.push_back(order);
//            publishTopOfBook(order.symbol, 'S', order.price, order.qty);
        } else if (order.price == orderBook.bestAsk.price) {
            orderBook.bestAsk.totalVolume += order.qty;
            orderBook.bestAsk.orders.push_back(order);
//            publishTopOfBook(order.symbol, 'S', order.price, orderBook.bestAsk.totalVolume);
        }
//        getTopOfBook(order.symbol, buy_order_books, sell_order_books, orderBook);
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


void handleCancelOrder(const Order& order, map<string, priority_queue<Order> >& buy_order_books,
                    map<string, priority_queue<Order> >& sell_order_books, vector<Trade>& trades,
                    mutex& orderBooksMutex, OrderBook &orderBook) {
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
                publishTopOfBuyBook(orderToDelete.symbol, tob, orderBook);
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
                publishTopOfSellBook(orderToDelete.symbol, tob, orderBook);
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

void handleMatch(const Order& order, map<string, priority_queue<Order> >& buy_order_books,
                    map<string, priority_queue<Order> >& sell_order_books, vector<Trade>& trades,
                    mutex& orderBooksMutex, OrderBook &orderBook) {
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

    // ----------------- Old Matching Logic -----------------
//    if (buy_order_books.count(order.symbol) && sell_order_books.count(order.symbol)) {
//        auto& buyOrders = buy_order_books[order.symbol];
//        auto& sellOrders = sell_order_books[order.symbol];
//        cout<<buyOrders.size()<<endl;
//        cout<<sellOrders.size()<<endl;
//
//        while (!buyOrders.empty() && !sellOrders.empty() && buyOrders.front().price >= sellOrders.front().price) {
//            int tradeQty = min(buyOrders.front().qty, sellOrders.front().qty);
//
//            Trade trade = {
//                    buyOrders.front().user,
//                    buyOrders.front().userOrderId,
//                    sellOrders.front().user,
//                    sellOrders.front().userOrderId,
//                    sellOrders.front().price,
//                    tradeQty
//            };
//            cout<< getTradeMessage(trade) << endl;
//
//            trades.push_back(trade);
//
//            buyOrders.front().qty -= tradeQty;
//            sellOrders.front().qty -= tradeQty;
//
//            if (buyOrders.front().qty == 0) {
//                buyOrders.pop_front();
//            }
//
//            if (sellOrders.front().qty == 0) {
//                sellOrders.pop_front();
//            }
//        }
//    }
}

void processOrder(const Order& order, map<string, priority_queue<Order> >& buy_order_books,
                  map<string, priority_queue<Order> >& sell_order_books, vector<Trade>& trades,
                  mutex& orderBooksMutex, OrderBook &orderBook) {
    lock_guard<mutex> lock(orderBooksMutex);
    if (order.type == OrderType::NEW) {
        handleNewOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
    } else if (order.type == OrderType::CANCEL) {
        handleCancelOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
    }
    handleMatch(order, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
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

void processRequest(string line, map<string, priority_queue<Order> > &buy_order_books,
                    map<string, priority_queue<Order> > &sell_order_books, vector<Trade> &trades, mutex& orderBooksMutex,
                    OrderBook &orderBook) {
    if (line.empty() || line[0] == '#') {
        return;
    }

    vector<string> inputLine;
    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
    istringstream iss(line);
    string cell;
    while (getline(iss, cell, ',')) {
        inputLine.push_back(cell);
//        cout<<cell<<"_";
    }
//    cout<< ":::::"<<inputLine.size()<<":::\n";


    if (inputLine[0] == "N") {
        Order order = { OrderType::NEW, stoi(inputLine[1]), stoi(inputLine[6]),
                        inputLine[2], stoi(inputLine[3]), stoi(inputLine[4]),
                        inputLine[5][0]};
        processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
        publishOrderAcknowledgement(order.user, order.userOrderId);
        getTopOfBook(inputLine[2], buy_order_books, sell_order_books, orderBook);
//        exit(0);
        return;
    } else if (inputLine[0] == "C") {
        Order order = { OrderType::CANCEL, stoi(inputLine[1]), stoi(inputLine[2]),
                        "", 0, 0, ' '};
        processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
        getTopOfBook(inputLine[2], buy_order_books, sell_order_books, orderBook);
    } else if (inputLine[0] == "F") {
        orderBook.flush();
        cout << "\n" << endl;
    }
}


int main() {
    ifstream file("input.csv");
    string line;

    OrderBook orderBook;

    map<string, priority_queue<Order> > buy_order_books;
    map<string, priority_queue<Order> > sell_order_books;
    vector<Trade> trades;
    mutex orderBooksMutex;

    while (getline(file, line)) {
        processRequest(line, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
        continue;
        if (line.empty() || line[0] == '#') {
            continue;
        }

        vector<string> inputLine;
        istringstream iss(line);
        string cell;
        while (getline(iss, cell, ',')) {
            inputLine.push_back(cell);
        }

        if (inputLine[0] == "N") {
            Order order = { OrderType::NEW, stoi(inputLine[1]), stoi(inputLine[6]),
                        inputLine[2], stoi(inputLine[3]), stoi(inputLine[4]),
                        inputLine[5][1]};
//            Order order = parseOrder(inputLine);
//            Order order(OrderType::NEW, std::stoi(inputLine[1]), std::stoi(inputLine[6]),
//                        inputLine[2], std::stoi(inputLine[3]), std::stoi(inputLine[4]),
//                        inputLine[5][0]);
//            cout<< "order.sideorder.sideorder.sideorder.side" <<order.side<< "...";
            processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
            publishOrderAcknowledgement(order.user, order.userOrderId);
//            cout<<"Symbol is"<<inputLine[2]<<endl;
//            getTopOfBook(inputLine[2], buy_order_books, sell_order_books, orderBook);
        } else if (inputLine[0] == "C") {
            Order order = { OrderType::CANCEL, stoi(inputLine[1]), stoi(inputLine[2]),
                        "", 0, 0, ' '};
            processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex, orderBook);
        } else if (inputLine[0] == "F") {
//            cout << "\n" << endl;
//            lock_guard<mutex> lock(orderBooksMutex);
//            for (const auto& [symbol, buyOrders] : buy_order_books) {
//                if (!buyOrders.empty()) {
////                    publishTopOfBook(symbol, 'B', buyOrders.front().price, buyOrders.front().qty);
//                    publishTopOfBook(symbol, 'B', buyOrders.front().price, buyOrders.front().qty);
//                } else {
//                    cout << "B,B,-,-" << endl;
//                }
//            }
//
//            for (const auto& [symbol, sellOrders] : sell_order_books) {
//                if (!sellOrders.empty()) {
//                    publishTopOfBook(symbol, 'S', sellOrders.front().price, sellOrders.front().qty);
//                } else {
//                    cout << "B,S,-,-" << endl;
//                }
//            }
        }
    }

    // Print trades
    for (const auto& trade : trades) {
        publishTrade(trade);
    }

    return 0;
}

