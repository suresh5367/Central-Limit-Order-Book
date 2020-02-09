//
// Created by Suresh kumar Reddy Ambati on 2/5/20.
//

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN  // in only one cpp file
#define BOOST_TEST_MODULE example
#include <boost/test/output_test_stream.hpp>
using boost::test_tools::output_test_stream;
#include <boost/test/unit_test.hpp>


#include "feed_handler.cpp"
#include "engine.cpp"


CS::OrderId id;
output_test_stream output;
CS::FeedHandler test_feed;
BOOST_AUTO_TEST_SUITE( BASIC_TEST )

    BOOST_AUTO_TEST_CASE(sample) {

    BOOST_CHECK_EQUAL("Hello fred", "Hello fred");
    BOOST_TEST(std::strcmp("test_string", "test_string") == 0);

}

BOOST_AUTO_TEST_CASE(Addorder) {
    //Add ORder1
    std::string order1("order 100002 sell 2 11");
    std::string query1("q order 100002");
    test_feed.ProcessMessage(order1);
    test_feed.PrintOrderBook(100002, output);
    BOOST_CHECK(output.is_equal("100002,1,2,11,Open"));
}

BOOST_AUTO_TEST_CASE(Invalid_Tick) {
    //Invalid Tick
    test_feed.ProcessMessage(string("order 19 sell 2 11.1"));

    test_feed.PrintOrderBook(19, output);
    BOOST_CHECK(output.is_equal("19 :Not present "));
}

BOOST_AUTO_TEST_CASE(Cancel_order_check_not_present) {
    //Cancel Order Check
    std::string cancquery1("cancel 100001");
    test_feed.ProcessMessage(cancquery1);
    CS::OrderId id=100001;
    test_feed.PrintOrderBook(100001, output);

        BOOST_CHECK( output.is_equal( "100001 :Not present " ) );
}

    BOOST_AUTO_TEST_CASE(Cancel_order_check) {
        //Cancel Order Check
        std::string cancquery1("cancel 100002");
        test_feed.ProcessMessage(cancquery1);
        CS::OrderId id=100002;
        test_feed.PrintOrderBook(100002, output);

        BOOST_CHECK( output.is_equal( "100002,1,2,11,Cancelled" ) );
    }
BOOST_AUTO_TEST_CASE(CancelTestcaseNotpresent) {


    {
        test_feed.PrintOrderBook(11111, output);
        BOOST_CHECK(output.is_equal("11111 :Not present "));
    }
}

BOOST_AUTO_TEST_CASE(Amend_order_up_size) {
    //Amend Order .Size up
        CS::FeedHandler test_feed2;
        test_feed2.ProcessMessage(std::string("order 1 sell 99 11"));
        test_feed2.ProcessMessage(std::string("order 2 sell 100 11"));
        test_feed2.ProcessMessage(std::string("order 3 sell 101 10"));
        test_feed2.ProcessMessage(std::string("order 4 sell 102 9"));

    test_feed2.ProcessMessage(std::string("amend 3 199"));  //Amend size up
    CS::Side s1 = CS::SELL;
    test_feed2.PrintLevelOrderBook(CS::SELL, 0, output);
    BOOST_CHECK(output.is_equal("ask,0,9,102\n"));

    test_feed2.PrintLevelOrderBook(CS::SELL, 1, output);
    BOOST_CHECK(output.is_equal("ask,1,10,199\n"));

}

    BOOST_AUTO_TEST_CASE(Cancel_order_check2) {
        //Cancel Order Check
        std::string cancquery1("cancel 100002");
        test_feed.ProcessMessage(cancquery1);
        test_feed.PrintOrderBook(100002, output);

        BOOST_CHECK( output.is_equal( "100002,1,2,11,Cancelled" ) );
    }

BOOST_AUTO_TEST_CASE(Amend_order_down_size) {
    //Amend Order .Size up
    test_feed.ProcessMessage(std::string("order 12345 buy 10 10"));
    test_feed.ProcessMessage(std::string("order 12346 buy 100 10"));
    test_feed.ProcessMessage(std::string("order 12347 buy 1000 10"));
        test_feed.ProcessMessage(std::string("order 12348 buy 9999 9"));
        test_feed.ProcessMessage(std::string("order 12349 buy 8888 8"));
    //1 ->[ {12345 , 10} , {12346 , 100} ,{12347 , 1000}]
    test_feed.ProcessMessage(std::string("amend 12345 9"));  //Amend size up
    CS::Side s1 = CS::SELL;
    test_feed.PrintLevelOrderBook(CS::BUY, 0, output);
    BOOST_CHECK(output.is_equal("bid,0,10,1109\n"));

    test_feed.PrintLevelOrderBook(CS::BUY, 1, output);
    BOOST_CHECK(output.is_equal("bid,1,9,9999\n"));

    test_feed.PrintLevelOrderBook(CS::BUY, 2, output);
    BOOST_CHECK(output.is_equal("bid,2,8,8888\n"));


}

BOOST_AUTO_TEST_CASE(Total_Orders) {


        BOOST_CHECK_EQUAL(test_feed.TotalOrders(),5);

}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( ADVANCED_TEST )
BOOST_AUTO_TEST_CASE(Execute) {
        CS::OrderId id;
        output_test_stream output;
        CS::FeedHandler test_feed;

        test_feed.ProcessMessage(std::string("order 100001 sell 20 11"));
        test_feed.ProcessMessage(std::string("order 100002 buy 20 11"));
        test_feed.PrintOrderBook(100001, output);
        BOOST_CHECK( output.is_equal( "100001,1,20,11,TradeFullyFilled" ) );
        test_feed.PrintOrderBook(100002, output);
        BOOST_CHECK( output.is_equal( "100002,0,20,11,TradeFullyFilled" ) );


        test_feed.ProcessMessage(std::string ("order 100001 sell 20 11"));
        test_feed.ProcessMessage(std::string ("order 100002 buy 30 12"));
        test_feed.ProcessMessage(std::string ("order 3 sell 10 12"));
        test_feed.ProcessMessage(std::string ("order 4 sell 20 11"));
        test_feed.ProcessMessage(std::string ("order 5 buy 10 11"));
        test_feed.PrintLevelOrderBook(CS::BUY, 0, output);
        BOOST_CHECK(output.is_equal(""));
        test_feed.PrintLevelOrderBook(CS::BUY, 1, output);
        BOOST_CHECK(output.is_equal(""));
        test_feed.PrintLevelOrderBook(CS::SELL, 0, output);
        BOOST_CHECK(output.is_equal("ask,0,11,10\n"));
        test_feed.PrintLevelOrderBook(CS::SELL, 1, output);
        BOOST_CHECK(output.is_equal("ask,1,12,10\n"));
}
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE( STOCK_GOING_UP )
/* b.txt
    order 2 sell 30 12
    order 3 sell 10 13
    order 4 sell 20 14
    order 5 buy 40 13*/
    BOOST_AUTO_TEST_CASE(Execute) {
        CS::OrderId id;
        output_test_stream output;
        CS::FeedHandler test_feed;

        test_feed.ProcessMessage(std::string("order 1 sell 30 12"));
        test_feed.ProcessMessage(std::string("order 2 sell 20 13"));
        test_feed.ProcessMessage(std::string("order 3 sell 20 14"));
        test_feed.ProcessMessage(std::string("order 4 buy 40 13"));

        test_feed.PrintOrderBook(1, output);
        BOOST_CHECK( output.is_equal( "1,0,0,11,TradeFullyFilled" ) );
        test_feed.PrintOrderBook(2, output);
        BOOST_CHECK( output.is_equal( "2,0,10,13,PartialFilled" ) );


        test_feed.ProcessMessage(std::string ("order 100001 sell 20 11"));
        test_feed.ProcessMessage(std::string ("order 100002 buy 30 12"));
        test_feed.ProcessMessage(std::string ("order 3 sell 10 12"));
        test_feed.ProcessMessage(std::string ("order 4 sell 20 11"));
        test_feed.ProcessMessage(std::string ("order 5 buy 10 11"));
        test_feed.PrintLevelOrderBook(CS::BUY, 0, output);
        BOOST_CHECK(output.is_equal(""));
        test_feed.PrintLevelOrderBook(CS::BUY, 1, output);
        BOOST_CHECK(output.is_equal(""));
        test_feed.PrintLevelOrderBook(CS::SELL, 0, output);
        BOOST_CHECK(output.is_equal("ask,0,11,10\n"));
        test_feed.PrintLevelOrderBook(CS::SELL, 1, output);
        BOOST_CHECK(output.is_equal("ask,1,12,10\n"));
    }
BOOST_AUTO_TEST_SUITE_END()

//To Do STOCK GOING DOWN(STRONG SELL ORDERS)
//This is Tested through File and needs to be put using Boost Unit Testing


//To Do Mix orders
//This is Tested through File and needs to be put using Boost Unit Testing


//To Do Bulk Orders
/*  This is Tested through File and needs to be put using Boost Unit Testing
 * order 12345 buy 10 10
order 12345 buy 10 11
order 12345 buy 10 12
order 12346 buy 100 10
order 12347 buy 1000 10
amend 12345 9
q level bid 0
order 100002 sell 2 11
order 19 sell 2 11.1
cancel 100002
q order 100002
order 100002 sell 2 11
q order 100002
order 19 sell 2 11.1
cancel 100002
order 100000 sell 10 10.5
order 100001 sell 20 10.5
order 100002 sell 2 11
cancel 100002
amend 100000 30
q level ask 0
q level ask 1
query order 100000


 * */



