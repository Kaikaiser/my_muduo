#include <iostream>
#include <string>
#include <string.h>
int add() 
{
    int a = 9;
    int b = 10;
    return a + b;
}
int main()
{
    int d = 0;
    std::string str = "nihaoa";
    const char* p = str.c_str();
    std::string str2(p);
    std::string str3 = p;
    std::cout<<p<<std::endl;
    std::cout<<str<<std::endl;
    std::cout<<str2<<std::endl;
    std::cout<<str3<<std::endl;
    //std::cout<<"hello world!"<<std::endl;
    //std::cout<<add()<<std::endl;
    //system("pause");
    return 0;
} 

