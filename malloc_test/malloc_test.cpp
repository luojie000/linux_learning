#include<iostream>
#include<vector>
#include<cstdlib>
#include<unistd.h>


int main()
{


std::vector<void*> v;
while(true)
{
void* p=malloc(1024*1024);
v.push_back(p);
std::cout<<"pid:"<<getpid()<<std::endl;
std::cout<<"allocated 1MB"<<std::endl;
sleep(1);
}
}