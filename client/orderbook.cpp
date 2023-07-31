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
#include <thread>
#include <mutex>
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
};
TopOfBook tob;

void publishTopOfBook2(const string& symbol, const TopOfBook& tob) {
    if (tob.bidQty > 0) {
        cout << "B,B," << tob.bidPrice << "," << tob.bidQty << endl;
    } else {
        cout << "B,B,-,-" << endl;
    }

    if (tob.askQty > 0) {
        cout << "B,S," << tob.askPrice << "," << tob.askQty << endl;
    } else {
        cout << "B,S,-,-" << endl;
    }
}

void publishTopOfBuyBook(const string& symbol, const TopOfBook& tob) {
    if (tob.bidQty > 0) {
        cout << "B,B," << tob.bidPrice << "," << tob.bidQty << endl;
    } else {
        cout << "B,B,-,-" << endl;
    }
}

void publishTopOfSellBook(const string& symbol, const TopOfBook& tob) {
    if (tob.askQty > 0) {
        cout << "B,S," << tob.askPrice << "," << tob.askQty << endl;
    } else {
        cout << "B,S,-,-" << endl;
    }
}

TopOfBook getTopOfBook(const string& symbol, map<string,deque<Order> > & buy_order_books,
map<string, deque<Order> > & sell_order_books) {

    bool isBuyChanged = false;
    bool isSellChanged = false;
    if (!buy_order_books[symbol].empty()) {
        if(buy_order_books[symbol].front().price != tob.bidPrice) {
            isBuyChanged = true;
        }
        tob.bidPrice = buy_order_books[symbol].front().price;
        tob.bidQty = buy_order_books[symbol].front().qty;
    } else {
        tob.bidPrice = tob.bidQty = 0;
//        isBuyChanged= true;
    }

    if (sell_order_books[symbol].size() > 1) {
        if(sell_order_books[symbol].front().price != tob.askPrice) {
            isSellChanged = true;
        }
        tob.askPrice = sell_order_books[symbol].back().price;
        tob.askQty = sell_order_books[symbol].back().qty;
    } else {
        tob.askPrice = tob.askQty = 0;
//        isSellChanged = true;
    }
    if(isSellChanged) {
        publishTopOfSellBook(symbol, tob);
    }
    if(isBuyChanged) {
        publishTopOfBuyBook(symbol, tob);
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

void handleNewOrder(const Order& order, map<string, deque<Order> >& buy_order_books,
                  map<string, deque<Order> >& sell_order_books, vector<Trade>& trades,
                  mutex& orderBooksMutex) {
    if (order.side == 'B') {
        buy_order_books[order.symbol].push_back(order);
    } else if (order.side == 'S') {
        sell_order_books[order.symbol].push_back(order);
    }
}

void handleCancelOrder(const Order& order, map<string, deque<Order> >& buy_order_books,
                    map<string, deque<Order> >& sell_order_books, vector<Trade>& trades,
                    mutex& orderBooksMutex) {
    if (order.side == 'B') {
        deque<Order>& buyOrders = buy_order_books[order.symbol];
        for (auto it = buyOrders.begin(); it != buyOrders.end(); ++it) {
            if (it->userOrderId == order.userOrderId) {
                buyOrders.erase(it);
                publishCancelAcknowledgement(order.user, order.userOrderId);
                break;
            }
        }
    } else if (order.side == 'S') {
        deque<Order>& sellOrders = sell_order_books[order.symbol];
        for (auto it = sellOrders.begin(); it != sellOrders.end(); ++it) {
            if (it->userOrderId == order.userOrderId) {
                sellOrders.erase(it);
                publishCancelAcknowledgement(order.user, order.userOrderId);
                break;
            }
        }
    }
}

void processOrder(const Order& order, map<string, deque<Order> >& buy_order_books,
                  map<string, deque<Order> >& sell_order_books, vector<Trade>& trades,
                  mutex& orderBooksMutex) {
    lock_guard<mutex> lock(orderBooksMutex);

    if (order.type == OrderType::NEW) {
        handleNewOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex);
//        if (order.side == 'B') {
//            buy_order_books[order.symbol].push_back(order);
//        } else if (order.side == 'S') {
//            sell_order_books[order.symbol].push_back(order);
//        }
    } else if (order.type == OrderType::CANCEL) {
        handleCancelOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex);
//        if (order.side == 'B') {
//            deque<Order>& buyOrders = buy_order_books[order.symbol];
//            for (auto it = buyOrders.begin(); it != buyOrders.end(); ++it) {
//                if (it->userOrderId == order.userOrderId) {
//                    buyOrders.erase(it);
//                    publishCancelAcknowledgement(order.user, order.userOrderId);
//                    break;
//                }
//            }
//        } else if (order.side == 'S') {
//            deque<Order>& sellOrders = sell_order_books[order.symbol];
//            for (auto it = sellOrders.begin(); it != sellOrders.end(); ++it) {
//                if (it->userOrderId == order.userOrderId) {
//                    sellOrders.erase(it);
//                    publishCancelAcknowledgement(order.user, order.userOrderId);
//                    break;
//                }
//            }
//        }
    }

    // Matching logic
    if (buy_order_books.count(order.symbol) && sell_order_books.count(order.symbol)) {
        auto& buyOrders = buy_order_books[order.symbol];
        auto& sellOrders = sell_order_books[order.symbol];

        while (!buyOrders.empty() && !sellOrders.empty() && buyOrders.front().price >= sellOrders.front().price) {
            int tradeQty = min(buyOrders.front().qty, sellOrders.front().qty);

            Trade trade = {
                    buyOrders.front().user,
                    buyOrders.front().userOrderId,
                    sellOrders.front().user,
                    sellOrders.front().userOrderId,
                    sellOrders.front().price,
                    tradeQty
            };

            trades.push_back(trade);

            buyOrders.front().qty -= tradeQty;
            sellOrders.front().qty -= tradeQty;

            if (buyOrders.front().qty == 0) {
                buyOrders.pop_front();
            }

            if (sellOrders.front().qty == 0) {
                sellOrders.pop_front();
            }
        }
    }
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
//    cout<<"inputLineinputLineinputLine"<<inputLine[5]<<endl;
    return order;
}

void processRequest(string line, map<string, deque<Order> > &buy_order_books, map<string, deque<Order> > &sell_order_books, vector<Trade> &trades, mutex& orderBooksMutex) {
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
        processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex);
        publishOrderAcknowledgement(order.user, order.userOrderId);
//        getTopOfBook(inputLine[2], buy_order_books, sell_order_books);
        return;
    } else if (inputLine[0] == "C") {
        Order order = { OrderType::CANCEL, stoi(inputLine[1]), stoi(inputLine[2]),
                        "", 0, 0, ' '};
        processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex);
    } else if (inputLine[0] == "F") {
        cout << "\n" << endl;
        lock_guard<mutex> lock(orderBooksMutex);
        for (const auto& [symbol, buyOrders] : buy_order_books) {
            if (!buyOrders.empty()) {
                publishTopOfBook(symbol, 'B', buyOrders.front().price, buyOrders.front().qty);
            } else {
                cout << "B,B,-,-" << endl;
            }
        }

        for (const auto& [symbol, sellOrders] : sell_order_books) {
            if (!sellOrders.empty()) {
                publishTopOfBook(symbol, 'S', sellOrders.front().price, sellOrders.front().qty);
            } else {
                cout << "B,S,-,-" << endl;
            }
        }
    }
}


int main() {
    ifstream file("input.csv");
    string line;

    map<string, deque<Order> > buy_order_books;
    map<string, deque<Order> > sell_order_books;
    vector<Trade> trades;
    mutex orderBooksMutex;

    while (getline(file, line)) {
//        cout<<"sell_order_bookssell_order_bookssell_order_books" <<sell_order_books.size()<<endl;
        processRequest(line, buy_order_books, sell_order_books, trades, orderBooksMutex);
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
            processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex);
            publishOrderAcknowledgement(order.user, order.userOrderId);
//            cout<<"Symbol is"<<inputLine[2]<<endl;
            getTopOfBook(inputLine[2], buy_order_books, sell_order_books);
        } else if (inputLine[0] == "C") {
            Order order = { OrderType::CANCEL, stoi(inputLine[1]), stoi(inputLine[2]),
                        "", 0, 0, ' '};
            processOrder(order, buy_order_books, sell_order_books, trades, orderBooksMutex);
        } else if (inputLine[0] == "F") {
            cout << "\n" << endl;
            lock_guard<mutex> lock(orderBooksMutex);
            for (const auto& [symbol, buyOrders] : buy_order_books) {
                if (!buyOrders.empty()) {
//                    publishTopOfBook(symbol, 'B', buyOrders.front().price, buyOrders.front().qty);
                    publishTopOfBook(symbol, 'B', buyOrders.front().price, buyOrders.front().qty);
                } else {
                    cout << "B,B,-,-" << endl;
                }
            }

            for (const auto& [symbol, sellOrders] : sell_order_books) {
                if (!sellOrders.empty()) {
                    publishTopOfBook(symbol, 'S', sellOrders.front().price, sellOrders.front().qty);
                } else {
                    cout << "B,S,-,-" << endl;
                }
            }
        }
    }

    // Print trades
    for (const auto& trade : trades) {
        publishTrade(trade);
    }

    return 0;
}

