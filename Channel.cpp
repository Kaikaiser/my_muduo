#include "Channel.h"



Channel(EventLoop *loop, int fd);
~Channel();

void handleEvent(TimeStamp receiveTime);