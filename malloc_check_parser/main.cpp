#include<iostream>
#include"MemoryParser.h"

//define the global
int global_a=10;
std::string global_b="hello world";
int main()
{
    int stack_a=3;
    std::cout<<"&a:"<<&stack_a<<"\n";
    int stack_b=5;
    int *p=new int(2);
    std::cout<<"&p:"<<p<<"\n";
    MemoryParser parser;
    parser.savememory();
    parser.showmemory();
    parser.caulatemessages();
    delete p;
    return 0;
}