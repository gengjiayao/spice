// main.cpp
#include "LUHelper.h"

using namespace std;

int main() {
    string csq_data_file = "/home/qingyangwang/Linux_Gutai2/jiayao/spice/strumpack/build/csq_data.txt";
    string sep_data_file = "/home/qingyangwang/Linux_Gutai2/jiayao/spice/strumpack/build/sep_data.txt";
    string tree_data_file = "/home/qingyangwang/Linux_Gutai2/jiayao/spice/strumpack/build/tree_data.txt";

    LUHelper app(csq_data_file, sep_data_file, tree_data_file);
    app.run();
    return 0;
}
