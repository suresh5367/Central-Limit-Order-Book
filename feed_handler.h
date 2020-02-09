//
// Created by Suresh on 4/2/20.
//

#ifndef CS_ORDERBOOK_FEED_HANDLER_H
#define CS_ORDERBOOK_FEED_HANDLER_H
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <string.h>
#include <vector>
#include "engine.h"
#include "error_monitor.h"

namespace CS {
    class FeedHandler
    {
    private:
        Engine engine_;

        enum class ParseResult {
            Good,
            CorruptMessage,
            InvalidMsgData
        };

        bool ParseTrade(char *tokenMsg, Trade &trd);

        bool ParseOrder(char *tokenMsg, Order &order);

        //Parse cancelOrder based on order-id
        bool ParseCancelOrder(char *tokenMsg, OrderId &orderId);

        bool ParseQueryOrder(char* tokenMsg, OrderId& orderid);

        bool ParseQueryLevelOrder(char* tokenMsg, Side& QuerySide, unsigned int& depth);

        bool ParseAmendOrder(char* tokenMsg, OrderId& orderId,TradeSize &AmendSize);

        void LogParseError(ParseResult err);

        std::vector<std::string> split (const std::string &s, char delim) ;

        ParseResult ParseTokenAsUInt64(char *&tk_msg, uint64_t &dest) {
            tk_msg = strtok(NULL, " ");
            if (tk_msg == NULL) {
                return ParseResult::CorruptMessage;
            }
            else {
                char * pEnd = NULL;
                dest = std::strtoull(tk_msg, &pEnd, 10);
                std::cout<<"setting"<<dest<<" from buffer\n";
                if (*pEnd)
                    return ParseResult::InvalidMsgData;
            }
            return ParseResult ::Good;
        }


        ParseResult ParseTokenAsUInt(char *&tk_msg, uint32_t &dest) {
            tk_msg = strtok(NULL, " ");
            if (tk_msg == NULL) {
                return ParseResult::CorruptMessage;
            }
            else {
                char * pEnd = NULL;
                dest = strtol(tk_msg, &pEnd, 10);
                if (*pEnd)
                    return ParseResult::InvalidMsgData;
            }
            return ParseResult ::Good;
        }

        ParseResult ParseSide(char *&tk_msg, uint32_t &dest) {
            tk_msg = strtok(NULL, " ");
            if (tk_msg == NULL) {
                return ParseResult::CorruptMessage;
            }
            else {
                if ( (strcmp(tk_msg , "sell")==0) || (strcmp(tk_msg, "ask") == 0)) {
                    dest = SELL;
                } else if ( (strcmp(tk_msg , "buy")==0) || (strcmp(tk_msg, "bid") == 0) ){
                    dest = BUY;
                } else {
                    return ParseResult::InvalidMsgData;
                }
            }
            return ParseResult ::Good;
        }

        ParseResult ParseQuery(char*& tk_msg, uint32_t& dest) {
            tk_msg = strtok(NULL, " ");
            if (tk_msg == NULL) {
                return ParseResult::CorruptMessage;
            }
            else {
                if (strcmp(tk_msg , "sell")==0) {
                    dest = SELL;
                }
                else if (strcmp(tk_msg , "buy")==0) {
                    dest = BUY;
                }
                else {
                    return ParseResult::InvalidMsgData;
                }
            }
            return ParseResult::Good;
        }
        /* To-DO for String parsing ,level or order
        ParseResult ParseString(char*& tk_msg, char*& dest) {
            tk_msg = strtok(NULL, " ");
            if (tk_msg == NULL) {
                return ParseResult::CorruptMessage;
            }
            else {
                char* pEnd = NULL;
                //dest = strtol(tk_msg, &pEnd, 10);
                dest = strtod(tk_msg, &pEnd);
                if (*pEnd)
                    return ParseResult::InvalidMsgData;
            }
            return ParseResult::Good;
        }
        */

        ParseResult ParsePrice(char *&tk_msg, double &dest) {
            tk_msg = strtok(NULL, " ");
            if (tk_msg == NULL) {
                return ParseResult::CorruptMessage;
            }
            else {
                char * pEnd = NULL;
                //dest = strtol(tk_msg, &pEnd, 10);
                dest = strtod (tk_msg, &pEnd);
                if (*pEnd)
                    return ParseResult::InvalidMsgData;
            }
            return ParseResult ::Good;
        }
        MessageType GetMessageType(char *tokenMsg) {
            uint32_t len = strlen(tokenMsg);
            //check length
            if (len == 0 || len > MaxMsgLength) {
                ErrorMonitor::GetInstance().CorruptMessage();
                return MessageType::Unknown;
            }

            tokenMsg = strtok(tokenMsg, " ");
            std::cout<<"tokenMsg:"<<tokenMsg<<std::endl;
            switch(tokenMsg[0]) {
                case 'o':


                    	std::cout<<"Adding \n";
                    	return MessageType::Add;


                    break;
                case 'a':
                	//if(strcmp(tokenMsg,"amend")==0)
                		return MessageType::Modify;
                    break;
                case 'c':
                	//if(strcmp(tokenMsg,"cancel")==0)
                	std::cout<<"cancelling \n";
                		return MessageType::Remove;
                    break;
                case 'T':
                    return MessageType::Trade;
                    break;
                case 'q':
                    return MessageType::Query;
                    break;
                default:
                    return MessageType::Unknown;
                    break;
            }
        }
    public:
        FeedHandler();
        void ProcessMessage(const std::string &line);
        void PrintCurrentOrderBook(std::ostream &os) const;
        void PrintLevelOrderBook(Side querySide, unsigned int depth,std::ostream &cout) const ;
        void PrintOrderBook(OrderId Id,std::ostream &os) const;
        unsigned int TotalOrders();

    };
}


#endif //CS_ORDERBOOK_FEED_HANDLER_H
