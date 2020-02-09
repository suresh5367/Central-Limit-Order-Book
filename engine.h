

#ifndef CS_ORDERBOOK_MATCH_ENGINE_H
#define CS_ORDERBOOK_MATCH_ENGINE_H

#include "types.h"
#include "order_book.h"


namespace CS {
    class Engine {
        HisOrderEntrys  hisOrderEntry_;
        HisOrderEntrys  CancelOrderEntry_;
        PriceOrderBook  buyOrderBook_;
        PriceOrderBook  sellOrderBook_;

        TradeSize       recentTradeSize_;
        Price           recentTradePrice_;

    private:
        void CheckAskBidPx();

    public:
        Engine();

        void HandleCancelOrder(OrderId cancelledorderid);

        void HandleQueryOrder(OrderId queryorderid);

        void HandleQueryLevelOrder(Side querySide,unsigned int depth,std::ostream &os) const;

        void HandleAmendOrder(OrderId Amendorderid, TradeSize Amendsize);

        void HandleOrder(const Order &order);

        void HandleTrade(const Trade &trade);

        void HandleMatchTrade(Order& order);

        void RetrieveOrder(OrderId orderid,std::ostream &os) const;

        bool TickValid(const Price& price);

        void PrintOrderBook(std::ostream &os) const;

        void PrintTradeMessage();

        unsigned int TotalOrders();

        void Add(const Order &order);
        void Remove(Order &oldOrder, const Order &newOrder);
        void Modify(Order &oldOrder, Order &newOrder);
        char* print_order_status_enum_string(OrderStatus status) const;
    };
}
#endif //CS_ORDERBOOK_MATCH_ENGINE_H
