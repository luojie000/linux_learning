#include"MemoryParser.h"
#include<iostream>
#include<fstream>
#include<sstream>
#include<string_view>

void MemoryParser::savememory()
{
    std::ifstream file("/proc/self/maps");
    if(!file.is_open())
    {
        std::cerr<<"the file can`t be opened\n";
        return;
    }
    std::string line;
    while(std::getline(file,line))
    {
       std::stringstream ss(line);
       std::string address;
       std::string permission;
       std::string offset;
       std::string device;
       std::string inode;
       std::string name;
       ss>>address
       >>permission
       >>offset
       >>device
       >>inode;
       std::getline(ss,name);
        while(!name.empty() && name[0]==' ')
        {
            name.erase(0,1);
        }
        regions.emplace_back(address,permission,name);
        if(name=="[heap]")
        {
            ++statistics.heapcount;
        }
        else if(name=="[stack]")
        {
            ++statistics.stackcount;
        }
        else if(permission.find('x')!=std::string::npos)
        {
            ++statistics.textcount;
        }
    }
    std::cout<<"all memoryregions have been saved\n\n";
}


void MemoryParser::showmemory()
{
    for(auto&c:regions)
    {
        std::cout<<c.getAddress()<<" "<<c.getPermission()<<" "<<c.getName()<<"\n";
    }
    std::cout<<"all messages have printed\n\n";
}

void MemoryParser::caulatemessages()
{
    std::cout<<"heap:"<<statistics.heapcount<<"\n";
    std::cout<<"stack:"<<statistics.stackcount<<"\n";
    std::cout<<"text:"<<statistics.textcount<<"\n\n";
}