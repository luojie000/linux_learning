#include<iostream>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

int main()
{
int value=10;
pid_t pid=fork();
if(pid<0)
{
    perror("fork");
    return 1;
}
else if(pid==0)
{
    value=20;
    std::cout<<"child pid:"<<getpid()<<"\n";
    std::cout<<"child value"<<value<<"\n";
    _exit(0);
}
else
{
    std::cout<<"parent pid:"<<getpid()<<"\n";
    std::cout<<"child pid:"<<pid<<"\n";
    std::cout<<"parent value:"<<value<<"\n";
    waitpid(pid,nullptr,0);
}
    return 0;
}