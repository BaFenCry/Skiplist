#include <iostream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <mutex>
#include <thread>
#include "./skiplist.h"

using namespace std;

int main()
{
    SkipList<int,std::string> *skiplist=new SkipList<int,std::string>(18,2);
    skiplist->start_scheduled_deletion(3);
    // skiplist->insert_element(10,"test10",2); 
    // skiplist->insert_element(20,"test20",2); 
    // skiplist->insert_element(30,"test30",2);
    // skiplist->display_list();
    // std::this_thread::sleep_for(std::chrono::seconds(3));
    // skiplist->display_list();
    skiplist->insert_element(10,"test10",5); 
    skiplist->insert_element(20,"test20",5); 
    skiplist->insert_element(30,"test30",5);
    // skiplist->display_list();
    std::this_thread::sleep_for(std::chrono::seconds(10));


    return 0;
}