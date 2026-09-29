#pragma once

#include<string>
class MemoryRegion
{
    private:
    std::string address;
    std::string permission;
    std::string name;
    public:
    MemoryRegion(const std::string &a,const std::string &b,const std::string &c):address(a),permission(b),name(c){}
    MemoryRegion()=default;
    std::string getAddress()
    {
        return address;
    }
    std::string getPermission()
    {
        return permission;
    }
    std::string getName()
    {
        return name;
    }
};