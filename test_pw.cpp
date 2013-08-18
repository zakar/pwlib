#include <stdio.h>
#include "pw.h"

using namespace PW;

static const int kMaxN = 100000000;

class AClient : public PWBase
{
public:
    virtual int Process() 
    {
        for (int i = 0; i < kMaxN; ++i);
        puts("A");
    }
};

class BClient : public PWBase
{
public:
    virtual int Process()
    {
        for (int i = 0; i < kMaxN; ++i);
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
