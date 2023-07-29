//
// Created by Chiranjeev Kumar on 7/29/23.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

using namespace std;

enum class Side {
    BUY,
    SELL
};

struct Order {
    int user;
    string symbol;
    int price;
    int qty;
    Side side;
    int userOrderId;
};

class OrderBook {
public:
    void processOrder(const Order& order) {
        if (order.side == Side::BUY) {
            bids[order.price].push_back(order);
            sort(bids[order.price].begin(), bids[order.price].end(), compareOrders);
        }
        else if (order.side == Side::SELL) {
            asks[order.price].push_back(order);
            sort(asks[order.price].begin(), asks[order.price].end(), compareOrders);
        }

        matchOrders();
    }

    void cancelOrder(int userOrderId) {
        for (auto& pair : bids) {
            for (auto it = pair.second.begin(); it != pair.second.end(); ++it) {
                if (it->userOrderId == userOrderId) {
                    pair.second.erase(it);
                    return;
                }
            }
        }

        for (auto& pair : asks) {
            for (auto it = pair.second.begin(); it != pair.second.end(); ++it) {
                if (it->userOrderId == userOrderId) {
                    pair.second.erase(it);
                    return;
                }
            }
        }
    }

    void printTopOfBook(ofstream& outputFile) {
        int bestBidPrice = bids.empty() ? 0 : bids.rbegin()->first;
        int bestBidQty = bids.empty() ? 0 : bids.rbegin()->second.front().qty;
        int bestAskPrice = asks.empty() ? 0 : asks.begin()->first;
        int bestAskQty = asks.empty() ? 0 : asks.begin()->second.front().qty;

        outputFile << "B, B, " << bestBidPrice << ", " << bestBidQty << endl;
        outputFile << "B, S, " << bestAskPrice << ", " << bestAskQty << endl;
    }

private:
    map<int, vector<Order>> bids;
    map<int, vector<Order>> asks;

    static bool compareOrders(const Order& o1, const Order& o2) {
        return o1.userOrderId < o2.userOrderId;
    }

    void matchOrders() {
        if (bids.empty() || asks.empty()) {
            return;
        }

        auto bidIt = bids.rbegin();
        auto askIt = asks.begin();

        while (bidIt != bids.rend() && askIt != asks.end()) {
            int bidPrice = bidIt->first;
            int askPrice = askIt->first;

            if (bidPrice < askPrice) {
                break;
            }

            vector<Order>& bidOrders = bidIt->second;
            vector<Order>& askOrders = askIt->second;

            int minQty = min(bidOrders.front().qty, askOrders.front().qty);

            printTrade(bidOrders.front(), askOrders.front(), minQty);

            if (bidOrders.front().qty == askOrders.front().qty) {
                bidOrders.erase(bidOrders.begin());
                askOrders.erase(askOrders.begin());
            }
            else if (bidOrders.front().qty > askOrders.front().qty) {
                bidOrders.front().qty -= askOrders.front().qty;
                askOrders.erase(askOrders.begin());
            }
            else {
                askOrders.front().qty -= bidOrders.front().qty;
                bidOrders.erase(bidOrders.begin());
            }

            if (bidOrders.empty()) {
                bids.erase(bidIt->first);
                bidIt = bids.rbegin();
            }

            if (askOrders.empty()) {
                asks.erase(askIt->first);
                askIt = asks.begin();
            }
        }
    }

    void printTrade(const Order& buyOrder, const Order& sellOrder, int qty) {
        cout << "T, " << buyOrder.user << ", " << buyOrder.userOrderId << ", "
             << sellOrder.user << ", " << sellOrder.userOrderId << ", "
             << buyOrder.price << ", " << qty << endl;
    }
};

int main() {
    ifstream inputFile("input.csv");
    ofstream outputFile("output.csv");

    if (!inputFile) {
        cout << "Error: Unable to open input file." << endl;
        return 1;
    }

    if (!outputFile) {
        cout << "Error: Unable to open output file." << endl;
        return 1;
    }

    OrderBook orderBook;

    string line;
    while (getline(inputFile, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ' ') {
            continue;
        }

        char action;
        int user, price, qty, userOrderId;
        string symbol, side;

        stringstream ss(line);
        ss >> action;

        if (action == 'N') {
            ss >> user >> symbol >> price >> qty >> side >> userOrderId;
            Side orderSide = (side == "B") ? Side::BUY : Side::SELL;
            Order order = { user, symbol, price, qty, orderSide, userOrderId };
            orderBook.processOrder(order);
        }
        else if (action == 'C') {
            ss >> user >> userOrderId;
            orderBook.cancelOrder(userOrderId);
        }
        else if (action == 'F') {
            orderBook.printTopOfBook(outputFile);
        }
    }

    inputFile.close();
    outputFile.close();

    return 0;
}

