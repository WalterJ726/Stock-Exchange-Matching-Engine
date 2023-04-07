#include "genXml.hpp"

int genXml::randomInt(int min, int max)
{
    return min + std::rand() % (max - min);
}

std::string genXml::generateRandomOrder(int account_id)
{
    std::string sym = generateRandomSym(account_sym.at(account_id - 1)); // Randomly choose a symbol from the account
    int amount = (rand() % max_symbol_shares + 1) * 100;

    if (rand() % 2 == 1)
    {
        amount *= -1;
    }
    int limit = rand() % LIMIT + 1;

    std::stringstream orderXml;
    orderXml << " <order sym=\"" << sym << "\" amount=\"" << amount << "\" limit=\"" << limit << "\"/>\n";
    return orderXml.str();
}

std::string genXml::generateRandomQuery(int transactionId)
{
    std::string queryXml = " <query id=\"" + std::to_string(transactionId) + "\"/>\n";
    return queryXml;
}

std::string genXml::generateRandomCancel(int transactionId)
{
    std::string cancelXml = " <cancel id=\"" + std::to_string(transactionId) + "\"/>\n";
    return cancelXml;
}

std::string genXml::generateRandomSym(const std::vector<std::string> &symList)
{
    int symIndex = rand() % symList.size();
    return symList[symIndex];
}

genXml::genXml() : accountnum(1), transnum(0)
{
}

std::string genXml::generateRandomCreateXml()
{
    std::cout << "enter generate()" << std::endl;
    std::ostringstream xml;
    xml << "1000\n"; // TODO: modify to number of bytes
    xml << "<create>\n";

    int num_accounts_f = rand() % max_account_f;
    int num_symbols = rand() % max_symbol;

    double balance = 0;
    // TODO: Will only create accounts in ascending order, but will not create already generated accounts.
    // Additional test examples for creating existing accounts need to be added
    for (int i = 0; i <= num_accounts_f; ++accountnum, ++i)
    {
        balance = rand() % max_balance + static_cast<double>(rand()) / RAND_MAX;
        xml << " <account id=\"" << accountnum << "\" balance=\"" << std::fixed << std::setprecision(2) << balance << std::fixed << std::setprecision(0) << "\"/>\n";
        account_sym[accountnum - 1];
    }
    std::vector<std::string> symbols{"AAPL", "GOOG", "AMZN"};
    for (int i = 0; i < num_symbols; ++i)
    {
        int n_symbols = rand() % symbols.size();
        std::string randomSym = symbols[n_symbols];
        symbols.erase(symbols.begin() + n_symbols);

        xml << " <symbol sym=\"" << randomSym << "\">\n";
        int num_symbol_accounts = rand() % max_account_c + 1;
        for (int j = 1; j <= num_symbol_accounts; ++j)
        {
            int value = (rand() % max_symbol_shares + min_symbol_shares) * 100;
            int random_acc = randomInt(1, accountnum);
            // 要这个账户下没有这个股票才可以create
            if (position[randomSym].find(random_acc) == position[randomSym].end())
            {
                xml << "  <account id=\"" << random_acc << "\">" << value << "</account>\n";
                // add symbol to account
                account_sym[random_acc - 1].push_back(randomSym);
                position[randomSym].insert(random_acc);
            }
        }
        xml << " </symbol>\n";
    }
    xml << "</create>";
    std::cout << xml.str() << std::endl;
    return xml.str();
}

std::string genXml::generateRandomTransXml()
{
    std::ostringstream xml;
    xml << "1000\n"; // TODO: modify to number of bytes
    int account_id = randomInt(1, accountnum);
    xml << "<transactions id=\"" << account_id << "\">\n";

    int nodeType = rand() % 2;
    // order
    if (account_sym[account_id - 1].size() != 0)
    {
        xml << generateRandomOrder(account_id);
        transnum++;
    }

    int transactionId;
    if (transnum > 0)
    {
        // query
        transactionId = rand() % transnum + 1;
        xml << generateRandomQuery(transactionId);

        // cancel
        transactionId = rand() % transnum + 1;
        xml << generateRandomCancel(transactionId);
    }

    xml << "</transactions>";
    std::cout << xml.str() << std::endl;
    return xml.str();
}