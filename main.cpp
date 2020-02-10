

#include <string>
#include <fstream>
#include <iostream>
#include "error_monitor.h"
#include "feed_handler.h"
using namespace std;

int main(int argc, char **argv)
{
    CS::FeedHandler feed;
    std::string line;
    int counter = 0;
    if (argc == 2)
    {

        std::string filename(argv[1]);

        std::cout << "parsing input file :"<<filename.c_str() << std::endl;
        std::ifstream infile(filename.c_str(), std::ios::in);




        while (std::getline(infile, line)) {
            cout<<"\n processing record#"<<counter<<" string:"<<line;
            feed.ProcessMessage(line);

            ++counter;

            if (counter % 10 == 0) {
                feed.PrintCurrentOrderBook(std::cout);

            }

        }
    }
    else
    {
        std::cout << "Input file not given\n.Please proceed with manual input of order\n.To Exit type EXIT or number";


        std::cout << "process from cmnnad lineinput\n:Enter as order 101 buy 123 10.5 :\n";

        std::getline(cin, line);
        while(1)
        {

            if( (line[0]>='0' && line[0]<='9') || line.compare("EXIT")==0 )
                break;
            cout<<"\n processing record#"<<counter<<" string:"<<line;
            feed.ProcessMessage(line);
            ++counter;
            std::cout << "process from comnnad line input\n:Enter order similar to order 101 buy 123 10.5  To Exit : Type Number or EXIT\n";
            std::getline(cin, line);
        }

    }


    feed.PrintCurrentOrderBook(std::cout);
    CS::ErrorMonitor::GetInstance().PrintStats();

    return 0;
}
