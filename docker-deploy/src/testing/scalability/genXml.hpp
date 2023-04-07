#include <iostream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <utility>
#include <set>

class genXml
{
private:
    const int max_account_f = 2;
    const int max_symbol = 4;
    const int max_account_c = 4;
    const int min_symbol_shares = 4;
    const int max_symbol_shares = 10;
    const int max_balance = 100000;
    const int LIMIT = 150;
    int accountnum; // need to initialize
    int transnum;
    std::map<int, std::vector<std::string>> account_sym;
    std::vector<int> trans;
    std::map<std::string, std::set<int>> position;

    int randomInt(int min, int max);

    std::string generateRandomOrder(int account_id);
    std::string generateRandomQuery(int transactionId);
    std::string generateRandomCancel(int transactionId);
    std::string generateRandomSym(const std::vector<std::string> &symList);

public:
    genXml();
    std::string generateRandomCreateXml();
    std::string generateRandomTransXml();
};