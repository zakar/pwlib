#include <stdio.h>
#include "pw.h"

using namespace PW;

class AClient : public PWBase
{
public:
    virtual int Process() 
    {
        puts("A");
    }
};

class BClient : public PWBase
{
public:
    virtual int Process()
    {
        puts("B");
    }
};

int main()
{
    AClient acli;
    BClient bcli;

    PWManager* mgr = PWManager::Instance();
    mgr->Init(2);
    mgr->Run(&acli);
    mgr->Run(&bcli);
    mgr->Wait();

    puts("finish");

    return 0;
}
