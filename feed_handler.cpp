//
// Created by Suresh on 4/2/20.
//

#include "feed_handler.h"
#include "order_book.h"
#include <sstream>
#include "error_monitor.h"

using namespace std;

namespace CS {
    FeedHandler::FeedHandler() {

    }

    vector<string> FeedHandler::split (const string &s, char delim) {
        vector<string> result;
        stringstream ss (s);
        string item;

        while (getline (ss, item, delim)) {
            result.push_back (item);
        }

        return result;
    }
    void FeedHandler::LogParseError(ParseResult err) {
        /*switch (err) {
            case ParseResult ::CorruptMessage:
                ErrorMonitor::GetInstance().CorruptMessage();
                break;
            case ParseResult ::InvalidMsgData:
                ErrorMonitor::GetInstance().InvalidMsg();
                break;
            default:
                break;
        }*/
        if(err==ParseResult::CorruptMessage)
            	{
            		ErrorMonitor::GetInstance().CorruptMessage();
            	}
            	else if (err==ParseResult::InvalidMsgData)
            	{
            		ErrorMonitor::GetInstance().InvalidMsg();
            	}
    }

    bool FeedHandler::ParseTrade(char *tokenMsg, Trade &trd) {
        ParseResult result = ParseTokenAsUInt(tokenMsg, trd.tradeSize);
        if (result != ParseResult::Good) {
            LogParseError(result);
            return false;
        }

        double price;
        result = ParsePrice(tokenMsg, price);
        if (result != ParseResult ::Good) {
            LogParseError(result);
            return false;
        }
        trd.tradePrice = price * 100;
        return true;
    }

    bool FeedHandler::ParseAmendOrder(char* tokenMsg, OrderId& orderId, TradeSize& AmendSize)
    {
        // order id
        ParseResult result = ParseTokenAsUInt64(tokenMsg, orderId);
        if (result != ParseResult::Good) {
            LogParseError(result);
            return false;
        }

        // size
        result = ParseTokenAsUInt(tokenMsg, AmendSize);
        if (result != ParseResult::Good) {
            LogParseError(result);
            return false;
        }

        return true;
    }
    bool FeedHandler::ParseCancelOrder(char *tokenMsg, OrderId &orderid) {
    	// order id
    	 ParseResult result = ParseTokenAsUInt64(tokenMsg, orderid);
    	 if (result != ParseResult ::Good){
    		 LogParseError(result);
    	     return false;
    	    }

    	 return true;

    }

    bool FeedHandler::ParseQueryOrder(char* tokenMsg, OrderId& orderid) {
        //order or level
        tokenMsg = strtok(NULL, " ");
        if (tokenMsg == NULL) {
            LogParseError(ParseResult::CorruptMessage);
            return false;
        }
        else if (strcmp(tokenMsg, "level") == 0) {
            return false;
        }
        else if((strcmp(tokenMsg, "order") == 0))
        {
            ; //TO-DO Works?
        }
        else
        {
            LogParseError(ParseResult::CorruptMessage);
            return false;
        }
        // order id
        ParseResult result = ParseTokenAsUInt64(tokenMsg, orderid);
        if (result != ParseResult::Good) {
            LogParseError(result);
            return false;
        }

        return true;

    }

    bool FeedHandler::ParseQueryLevelOrder(char* tokenMsg,  Side& QuerySide,unsigned int& depth) {

        
          
        // side
        ParseResult result = ParseSide(tokenMsg, QuerySide);
        if (result != ParseResult::Good) {
            LogParseError(result);
            return false;
        }

        // depth
        result = ParseTokenAsUInt(tokenMsg, depth);
        if (result != ParseResult::Good) {
            LogParseError(result);
            return false;
        }

        return true;

    }

    void FeedHandler::PrintOrderBook(OrderId Id,std::ostream &os) const
    {
        return engine_.RetrieveOrder(Id,os);

    }
    unsigned int FeedHandler::TotalOrders()
    {
        return engine_.TotalOrders();

    }
    bool FeedHandler::ParseOrder(char *tokenMsg, Order &order) {

            // order id
            ParseResult result = ParseTokenAsUInt64(tokenMsg, order.orderId);
            if (result != ParseResult ::Good){
                LogParseError(result);
                return false;
            }

            // side
            result = ParseSide(tokenMsg, order.side);
            if (result != ParseResult ::Good) {
                LogParseError(result);
                return false;
            }

            // size
            result = ParseTokenAsUInt(tokenMsg, order.size);
            if (result != ParseResult::Good) {
                LogParseError(result);
                return false;
            }

            if (order.size > MaxTradeSize) {
                result = ParseResult ::InvalidMsgData;
                LogParseError(result);
                return false;
            }

            // price
            double price;
            result = ParsePrice(tokenMsg, price);
            if (result != ParseResult ::Good) {
                LogParseError(result);
                return false;
            }
            order.price = price;

            if (order.price > MaxTradePrice) {
                result = ParseResult ::InvalidMsgData;
                LogParseError(result);
                return false;
            }
            order.status = OrderStatus::Normal;
            return true;
        }


    void FeedHandler::ProcessMessage(const std::string &line) {
            static Order order;
            static OrderId cancelorderId;
            static OrderId queryorderId;
            static OrderId AmendorderId;
            static TradeSize AmendorderSize;
            static Trade trd;
            static Side querySide;
            static unsigned int depth;


            char *buf = const_cast<char*>(line.c_str());
            MessageType mt = GetMessageType(buf);

            if (mt == MessageType::Unknown) {
                ErrorMonitor::GetInstance().CorruptMessage();
            }  else {
                order.action = mt;

           if (mt == MessageType::Remove) {

        	   if(ParseCancelOrder(buf,cancelorderId))
        	   {
        	       //tokenMsg:otokenMsg:o

        		   engine_.HandleCancelOrder(cancelorderId);
        	   }
        	   else
                   ErrorMonitor::GetInstance().InvalidMsg();
           }
           else if(mt==MessageType::Modify)
           {


               if (ParseAmendOrder(buf, AmendorderId, AmendorderSize))
               {

                   engine_.HandleAmendOrder(AmendorderId, AmendorderSize);
               }
               else
               {
                   //order not present
                   ErrorMonitor::GetInstance().InvalidMsg();
               }

           }
           else if(mt==MessageType::Add)
           {
                if (ParseOrder(buf, order)) {
                    engine_.HandleOrder(order);
                } else {
                    ErrorMonitor::GetInstance().CorruptMessage();
                }
           }
           else if (mt == MessageType::Query)
           {
               if (ParseQueryOrder(buf, queryorderId))
               {
                   engine_.HandleQueryOrder(queryorderId);
               }
               else if (ParseQueryLevelOrder(buf, querySide,depth))
               {
                   engine_.HandleQueryLevelOrder(querySide,depth,std::cout);
               }              
               else {
                   ErrorMonitor::GetInstance().InvalidMsg();
               }
            
           }
            }

        }



    void FeedHandler::PrintLevelOrderBook(Side querySide, unsigned int depth,std::ostream &cout) const {
        engine_.HandleQueryLevelOrder(querySide,depth,cout);
    }


    void FeedHandler::PrintCurrentOrderBook(std::ostream &os) const {
       // engine_.PrintOrderBook(os);
    }
}
