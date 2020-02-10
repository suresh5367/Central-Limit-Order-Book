//
// Created by Suresh on 4/2/20.
//
#pragma warning (disable : 4146)
#include <climits>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <iomanip>

#include "engine.h"
#include "error_monitor.h"
#include "order_book.h"


using namespace std;

namespace CS {
    Engine::Engine()
            :recentTradeSize_(0),recentTradePrice_(0)
    {
    }

    bool Engine::TickValid(const Price& price)
    {
        float q=price*2;
        if( ( floor(q)-q)!=0)
        {
            return 0;
        }
        else
        {
            return 1;
        }

    }

    void Engine::HandleCancelOrder(OrderId cancelledorderid)
    {

        auto &&oldOrderIt = hisOrderEntry_.find(cancelledorderid);
        //some entry should be there
        if (oldOrderIt != hisOrderEntry_.end()) {
            Order &CancelOrder = oldOrderIt->second;
            CancelOrder.action=MessageType::Remove;
            HandleOrder(CancelOrder);

        }
    }

    void Engine::HandleQueryOrder(OrderId queryorderid)
    {

        auto&& queryOrderIt = hisOrderEntry_.find(queryorderid);
        //some entry should be there
        if (queryOrderIt != hisOrderEntry_.end()) {
            std::cout << queryorderid<<" is :"<<print_order_status_enum_string(queryOrderIt->second.status);
        }
        else
        {
            auto&& querycancelOrderIt = CancelOrderEntry_.find(queryorderid);
            if (querycancelOrderIt != CancelOrderEntry_.end()) {
                std::cout << queryorderid<<" is :"<<print_order_status_enum_string(querycancelOrderIt->second.status);
            } else
            {
                ErrorMonitor::GetInstance().CorruptMessage();
            }
        }
    }

    void Engine::HandleQueryLevelOrder(Side querySide, unsigned int depth,std::ostream &cout) const
    {

        if (depth < 0)
        {
            std::cout << "Error in message query.Level depth is negative \n";
            ErrorMonitor::GetInstance().CorruptMessage();
            return;
        }

        if (querySide == BUY)
        {
            auto LevelIter = buyOrderBook_.rbegin();
            for (int i=depth;LevelIter != buyOrderBook_.rend() && i != 0;++LevelIter, --i)
                ;
            if (LevelIter != buyOrderBook_.rend())
            {
                cout << "bid," << depth<<","<< LevelIter->first<<"," << LevelIter->second.GetTradeSize() << endl;
                return;
            }
            else
            {
                std::cout << "Error in message query.Level depth incorrect \n";
                ErrorMonitor::GetInstance().CorruptMessage();
                return;
            }
        }
        else
        {

            auto LevelIter = sellOrderBook_.begin();
            for (int i = depth;LevelIter != sellOrderBook_.end() && i != 0;++LevelIter, --i)
                ;
            if (LevelIter != sellOrderBook_.end())
            {
                cout << "ask," << depth << "," << LevelIter->first << "," << LevelIter->second.GetTradeSize() << endl;
                return;
            }
            else
            {
                std::cout << "Error in message query.Level depth incorrect \n";
                ErrorMonitor::GetInstance().CorruptMessage();
                return;
            }

        }

    }

    void Engine::HandleAmendOrder(OrderId Amendorderid, TradeSize Amendsize)
    {
        auto&& oldOrderIt = hisOrderEntry_.find(Amendorderid);
        //some entry should be there
        if (oldOrderIt != hisOrderEntry_.end()) {
            Order AmendOrder = oldOrderIt->second;
            AmendOrder.action = MessageType::Modify;
            AmendOrder.size = Amendsize;

            HandleOrder(AmendOrder);

        }
    }
    char* Engine::print_order_status_enum_string(OrderStatus status) const
    {
        int st=(int) status;
        switch(st)
        {
            case 0 : return "Open"; break;
            case 1 : return "TradeFullyFilled"; break;
            case 2 : return "PartialFilled"; break;
            case 3: return "Cancelled"; break;
        }
    }
    void Engine::RetrieveOrder(OrderId orderid,std::ostream &os) const
    {

        auto&& OrderIt = hisOrderEntry_.find(orderid);

        if (OrderIt != hisOrderEntry_.end()) {
            Order retrieveOrder = OrderIt->second;
            os<<retrieveOrder.orderId<<","<<retrieveOrder.side<<","<<retrieveOrder.size<<","<<retrieveOrder.price<<","<<print_order_status_enum_string(retrieveOrder.status);
        }
        else
        {
            auto&& querycancelOrderIt = CancelOrderEntry_.find(orderid);
            if (querycancelOrderIt != CancelOrderEntry_.end()) {
                Order retrieveOrder = querycancelOrderIt->second;
                os<<retrieveOrder.orderId<<","<<retrieveOrder.side<<","<<retrieveOrder.size<<","<<retrieveOrder.price<<","<<print_order_status_enum_string(retrieveOrder.status);
            } else
            {
                os<<orderid<<" :Not present ";
            }
        }

    }
    void Engine::HandleOrder(const Order &order) {
        auto &&oldOrderIt = hisOrderEntry_.find(order.orderId);

        if (oldOrderIt == hisOrderEntry_.end()) {
            if (order.action == MessageType ::Add) {
                if(TickValid(order.price))
                    Add(order);
                else
                {
                    cout<<"Invalid Tick price :"<< order.price<<"in order id:"<<order.orderId<<std::endl;
                    ErrorMonitor::GetInstance().RemoveWithWrongData();
                }
            } else {
                // missing order
                ErrorMonitor::GetInstance().CancelMissingOrder();
            }
        } else {
            // order exist
            Order &oldOrder = oldOrderIt->second;

            if (order.action == MessageType::Add) {
                // duplicate add
                ErrorMonitor::GetInstance().DuplicateAdd();

            } else if (order.action == MessageType ::Remove) {

                Remove(oldOrder, order);

            } else if (order.action == MessageType ::Modify) {
                //skip if same size
                if (oldOrder.size == order.size) {
                    //it is just a notification after trade match or
                    //it is intend to modify but without change, ignore it too.
                    return;
                }

                Modify(oldOrder, const_cast<Order&>(order));

            } else {
                //TODO not possible here

            }
        }
    }

    void Engine::CheckAskBidPx() {
        if (buyOrderBook_.empty() || sellOrderBook_.empty()) {
            return;
        }
        auto &&sellIter = sellOrderBook_.begin();
        auto &&buyIter = buyOrderBook_.rbegin();
        if (sellIter->first <= buyIter->first) {
            ErrorMonitor::GetInstance().CrossBidAsk();
        }
    }

    //
    void Engine::Modify(Order &oldOrder, Order &newOrder) {
        CheckAskBidPx();

        if (oldOrder.status == OrderStatus::TradeDeleted) {
            ErrorMonitor::GetInstance().modifyOrderDeleted();
            return;
        }


        if (oldOrder.size == newOrder.size) {
            // this is either modify order but without any change or
            // after trade happened, an exchange message to reflect the order book/hisentry
            // change(which was done in HandleTrade)
            ErrorMonitor::GetInstance().ModifyIgnored();
            return;
        }

        if (newOrder.size>oldOrder.size) //if the size has increased in Amendment ,then remove the old one and add it as new one,changes time prioirty
        {
            // remove old order
            Remove(oldOrder, oldOrder);
            Add(newOrder);
        } else {  //add check newOrder.size<oldOrder.size to be on safer side
            // same side, same price, diff size of low , keep time priority
            // TODO if size = 0
            if (newOrder.side == BUY) {
                auto &&buyOrderList = buyOrderBook_.find(oldOrder.price);
                if (buyOrderList != buyOrderBook_.end()) {
                    buyOrderList->second.DecreaseTradeSize(oldOrder.size-newOrder.size);
                    oldOrder.ptr->size = newOrder.size;
                } else {
                    ErrorMonitor::GetInstance().modifyOrderDeleted();
                    return;
                }
            } else if (newOrder.side == SELL) {
                auto &&sellOrderList = sellOrderBook_.find(oldOrder.price);
                if (sellOrderList != sellOrderBook_.end()) {
                    sellOrderList->second.DecreaseTradeSize(oldOrder.size-newOrder.size);
                    oldOrder.ptr->size = newOrder.size;
                } else {
                    ErrorMonitor::GetInstance().modifyOrderDeleted();
                    return;
                }
            }
        }
    }

    void Engine::Remove(Order &oldOrder, const Order &newOrder) {
        CheckAskBidPx();

        // to remove order, their price, size, side must be equal.
        if (oldOrder.status == OrderStatus::TradeDeleted) {
            std::cout<<"Trade already deleted\n";
            ErrorMonitor::GetInstance().RemoveWithWrongData();

            return;
        }


        // if the order status is 'trade delete' means we don't need to modify order book
        if (oldOrder.status != OrderStatus::TradeDeleted) {
            PriceOrderBook &map = oldOrder.side == BUY ? buyOrderBook_ : sellOrderBook_;
            auto &&iter = map.find(oldOrder.price);

            if (iter == map.end()) {
                ErrorMonitor::GetInstance().RemoveWithWrongData();
            } else {
                auto &&ordList = iter->second;
                ordList.erase(oldOrder.ptr);
                // change order list total size
                ordList.ChangeTradeSize(-oldOrder.size);
                //Also change Orderstatus to Delete
                oldOrder.status=OrderStatus::Cancelled;

                if (ordList.empty()) {
                    map.erase(iter);
                }
            }
        }

        // remove from hisorderentry. but maintain a sepearte cancelled orders books
        auto &&oldOrderIt = hisOrderEntry_.find(oldOrder.orderId);
        if (oldOrderIt != hisOrderEntry_.end()) {
            hisOrderEntry_.erase(oldOrderIt);
            CancelOrderEntry_.insert(std::make_pair(oldOrder.orderId,oldOrder));

        }
    }

    unsigned int Engine::TotalOrders()
    {
        return hisOrderEntry_.size();

    }

    void Engine::Add(const Order &tmpOrder) {
        CheckAskBidPx();

        

        auto &&ret = hisOrderEntry_.insert(std::make_pair(tmpOrder.orderId, tmpOrder));
        Order &order = ret.first->second;

        // put in the book
        PriceOrderBook &map = order.side == BUY? buyOrderBook_ : sellOrderBook_;
        auto &&iter = map.find(order.price);
        if (iter!=map.end()) {
            iter->second.push_back(PriceOrderItem(order.size, order.orderId));
            iter->second.ChangeTradeSize(order.size);
            order.ptr = iter->second.end();
            order.ptr--;
        } else {
            auto &&ret = map.insert(std::make_pair(order.price, SamePriceOrderList()));
            ret.first->second.push_back(PriceOrderItem(order.size, order.orderId));
            ret.first->second.ChangeTradeSize(order.size);
            order.ptr = ret.first->second.end();
            order.ptr--;
        }
        HandleMatchTrade(order);
    }

    void Engine::HandleMatchTrade(Order& new_order)
    {
        if (buyOrderBook_.empty() || sellOrderBook_.empty()) {
            ErrorMonitor::GetInstance().TradeOnMissingOrder();
            return;
        }

        //new_order is a buy
        if (new_order.side == 0)
        {
            //check the highest price on Buy side i.e last item in buy order book
            auto&& highestBuyIter = buyOrderBook_.end();
            highestBuyIter--;

            auto&& highestSellIter = sellOrderBook_.begin();

            Trade match_trade;
            vector<CS::Trade> matched_trades;
            match_trade.tradePrice = 0;
            match_trade.tradeSize = 0;
            TradeSize totalfill = 0;
            //check for the highest price on buy side , if we have any sell orders
            //to do ->put this in a loop.case : for new Buy order ,there can be more than one sell orders
            for (;
                    highestSellIter != sellOrderBook_.end() &&
                    highestBuyIter->first >= highestSellIter->first  &&
                    totalfill < new_order.size ;
                    highestSellIter++)
            {


                if (highestSellIter->second.GetTradeSize() > highestBuyIter->second.GetTradeSize())
                {
                    if ((new_order.size - totalfill) > highestBuyIter->second.GetTradeSize())
                        match_trade.tradeSize = highestBuyIter->second.GetTradeSize();
                    else
                        match_trade.tradeSize = (new_order.size - totalfill);

                    totalfill += match_trade.tradeSize;
                }
                else
                {
                    if ((new_order.size - totalfill) > highestSellIter->second.GetTradeSize())
                        match_trade.tradeSize = highestSellIter->second.GetTradeSize();
                    else
                        match_trade.tradeSize = (new_order.size - totalfill);

                    totalfill += match_trade.tradeSize;
                }
                match_trade.tradePrice=highestBuyIter->first;
                matched_trades.push_back(match_trade);


            }

            for (auto matchedTradesitr= matched_trades.begin();matchedTradesitr!= matched_trades.end();matchedTradesitr++)
            {
                if ((matchedTradesitr->tradePrice > 0) && matchedTradesitr->tradeSize > 0)
                {
                    HandleTrade(*matchedTradesitr);
                }
            }
            matched_trades.clear();
        }
        else //it is a new SELL order
        {
            //check the lowest price on Sell side i.e first item in buy order book
            auto&& highestSellIter = sellOrderBook_.begin();

            //get highest buy order


            auto&& highestBuyIter = buyOrderBook_.rbegin();

            vector<CS::Trade> sell_matched_trades;

            TradeSize totalfill = 0;
            Trade sell_match_trade;
            sell_match_trade.tradePrice = 0;
            sell_match_trade.tradeSize = 0;
            //check for the highest price on buy side , if we have any sell orders
            //to do ->put this in a loop.case : for new Buy order ,there can be more than one sell orders
            //highestSellIter->first <= highestBuyIter->first
            for (;
                    highestBuyIter != buyOrderBook_.rend() &&
                    highestSellIter->first <= highestBuyIter->first &&
                    sell_match_trade.tradeSize < new_order.size;
                    highestBuyIter++)
            {

                /* Existing Logic
                match_trade.tradeSize = new_order.size;
                match_trade.tradePrice = highestBuyIter->first;
                */
                if (highestSellIter->second.GetTradeSize() > highestBuyIter->second.GetTradeSize())
                {
                    if ((new_order.size - totalfill) > highestBuyIter->second.GetTradeSize())
                        sell_match_trade.tradeSize = highestBuyIter->second.GetTradeSize();
                    else
                        sell_match_trade.tradeSize = (new_order.size - totalfill);

                    totalfill += sell_match_trade.tradeSize;
                }
                else
                {
                    if ((new_order.size - totalfill) > highestSellIter->second.GetTradeSize())
                        sell_match_trade.tradeSize = highestSellIter->second.GetTradeSize();
                    else
                        sell_match_trade.tradeSize = (new_order.size - totalfill);

                    totalfill += sell_match_trade.tradeSize;
                }
                sell_match_trade.tradePrice = highestSellIter->first;
              
                sell_matched_trades.push_back(sell_match_trade);



            }

            for (auto matchedTradesitr= sell_matched_trades.begin();matchedTradesitr!= sell_matched_trades.end();matchedTradesitr++)
            {
                if ((matchedTradesitr->tradePrice > 0) && matchedTradesitr->tradeSize > 0)
                {
                    HandleTrade(*matchedTradesitr);
                }
            }
            sell_matched_trades.clear();

        }

    }
    void Engine::HandleTrade(const Trade &trade) {
        // Not check bid and ask price
        // ensure there is cross book?
        if (buyOrderBook_.empty() || sellOrderBook_.empty()) {
            ErrorMonitor::GetInstance().TradeOnMissingOrder();
            return;
        }

        auto &&buyIter = buyOrderBook_.end();
        buyIter--;
        if (buyIter->first < trade.tradePrice) {
            ErrorMonitor::GetInstance().TradeOnMissingOrder();
            return;
        }

        

        auto &&sellIter = sellOrderBook_.begin();
        //try this
        if (sellIter == sellOrderBook_.end() ||
            sellIter->first>trade.tradePrice) {
            //The price Bid may be greater then offer price or vice versa.
            //ex:sell @ 13 order present and new order with buy came at 14.
            ErrorMonitor::GetInstance().TradeOnMissingOrder();
            return;

        }


        

        //ensure enough orderlist quantity
        TradeSize  ts = trade.tradeSize;
        auto &&sellOrderList = sellIter->second;
        auto &&buyOrderList = buyIter->second;
        /*TO-DO .
        for this scenario
        order 100000 sell 10 10.5
        order 100002 buy 20 10.5

        the buy order with 20 should fill(match) with the sell of 10.
        this below check is returning .check(Test) after removing

        but this one is worked.
        order 100000 sell 30 10.5
        order 100002 buy 20 10.5,bcoz next(new) order size is more(or eqaul) than the existing(previous) size
        */
        if (buyOrderList.GetTradeSize() < trade.tradeSize ||
            sellOrderList.GetTradeSize() < trade.tradeSize) {
            ErrorMonitor::GetInstance().TradeOnMissingOrder();
            return;
        } else {
            sellOrderList.ChangeTradeSize(-ts);
            buyOrderList.ChangeTradeSize(-ts);
        }



        // sell order book
        auto &&curSellItem = sellOrderList.begin();
        auto &&endSellItem = sellOrderList.end();

        while (curSellItem != endSellItem) {
            auto &&orignOrd = this->hisOrderEntry_.find(curSellItem->orderId);
            if (orignOrd!=hisOrderEntry_.end()) {
                if (ts >= curSellItem->size) {
                    // mark order in hisorderentry as trade delete
                    orignOrd->second.TradeDelete();
                } else {
                    // change the hisorderentry order size
                    orignOrd->second.size -= ts;
                    //added new here
                    orignOrd->second.TradePartialFilled();
                }
            } else {
                ErrorMonitor::GetInstance().TradeOnMissingOrder();
            }

            if (ts >= curSellItem->size) {
                ts -= curSellItem->size;
                ++curSellItem;
            } else {
                curSellItem->size -= ts;
                ts = 0;
                break;
            }
        }
        sellOrderList.erase(sellOrderList.begin(), curSellItem);
        if (sellOrderList.empty()) {
            sellOrderBook_.erase(sellIter);
        }

        // buy order book
        ts = trade.tradeSize;

        auto &&curBuyItem = buyOrderList.begin();
        auto &&endBuyItem = buyOrderList.end();

        while (curBuyItem != endBuyItem) {
            auto &&orignOrd = this->hisOrderEntry_.find(curBuyItem->orderId);
            if (orignOrd!=hisOrderEntry_.end()) {
                if (ts >= curBuyItem->size) {
                    // mark order in hisorderentry as trade delete
                    orignOrd->second.TradeDelete();
                    //also same as FULFILLED
                } else {
                    // change the hisorderentry order size
                    orignOrd->second.size -= ts;
                    //change status as partialFill
                    //added new here
                    orignOrd->second.TradePartialFilled();
                }
            } else {
                ErrorMonitor::GetInstance().TradeOnMissingOrder();
            }

            if (ts >= curBuyItem->size) {
                ts -= curBuyItem->size;
                ++curBuyItem;
            } else {
                curBuyItem->size -= ts;
                ts = 0;
                break;
            }
        }
        buyOrderList.erase(buyOrderList.begin(),curBuyItem);


        if (buyOrderList.empty()) {
            buyOrderBook_.erase(buyIter);
        }

        // print recent trade message =================================
        if (recentTradePrice_ != trade.tradePrice) {
            recentTradePrice_ = trade.tradePrice;
            recentTradeSize_ = 0;
        }
        recentTradeSize_ += trade.tradeSize;
#ifndef PROFILE
        std::cout << "--- TRADE --------------------------------------\n";
        std::cout << trade.tradeSize << "@" << (double)(recentTradePrice_)<<".   Total Tradesize:"<<recentTradeSize_ << "@" << (double)(recentTradePrice_) << endl;
#endif
    }



    void Engine::PrintOrderBook(std::ostream &os) const {
        os << "---ORDER BOOK --------------------------------------\n";
        for (auto it = sellOrderBook_.rbegin(); it != sellOrderBook_.rend(); ++it) {
            auto &&lst = it->second;
            os << std::setw(10) << it->first << " ";
            for (auto &&sz : lst) {
                os << " S " << sz.size;
            }
            os << endl;
        }

        for (auto it = buyOrderBook_.rbegin(); it != buyOrderBook_.rend(); ++it) {
            auto &&lst = it->second;
            os << std::setw(10) << it->first << " ";
            for (auto &&sz : lst) {
                os << " B " << sz.size;
            }
            os << endl;
        }
    }

}
