#include "Thread.h"

Thread(ThreadFunc, const std::string &name_ = string())
{

}

~Thread()
{

}

void start()
{

}
void join()
{


}
bool started() const {return started_;}
bool joined() const {return joined_;}
pid_t tid() const {return tid_;}
const std::sting& name() const {return name_;}
static int numCreated() {return numCreated_;}


void setDefalutName()
{
    
}