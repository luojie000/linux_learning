#include<iostream>
#include<unistd.h>


int global_a;
int global_b=100;
int global_c=50;

void func()
{

}
int main()
{
    int stack_var=10;
    int *heap_var=new int(20);
    std::cout<<"PID:"<<getpid()<<std::endl;

    std::cout<<"global_a address:"<<&global_a<<std::endl;
    std::cout<<"global_b address:"<<&global_b<<std::endl;
    std::cout<<"global_c address:"<<&global_c<<std::endl;
    std::cout<<"stack address:"<<&stack_var<<std::endl;
    std::cout<<"heap address:"<<heap_var<<std::endl;

    getchar();
    delete heap_var;
    return 0;
}