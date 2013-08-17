
namespace PW
{

enum {
    PW_OK = 0,
    PW_ERR_LIMIT = 1,
    PW_ERR_SYS = -1,
};

struct PWCtx;

class PWBase
{
public:
    virtual int Process() = 0;
};

class PWManager
{
public:
    static PWManager* Instance();
    int Init(int count);
    int Run(PWBase* worker);
    int Wait();
private:
    int Create();
    PWManager();
    ~PWManager();
    PWCtx* _ctx; 
};

}
