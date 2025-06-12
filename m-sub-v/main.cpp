#include "systemc.h"
 
SC_MODULE(hello)
{
    SC_CTOR(hello)
    {
        cout<<"Hello, SystemC!"<<endl;
    }
};
  
//main.cpp
int sc_main(int i, char* a[])
{
    hello h("hello");
    return 0;
}