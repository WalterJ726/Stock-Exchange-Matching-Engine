#include "Database.hpp"

#include <sstream>
#include <vector>
using pqxx::work;
using std::stringstream;
using std::vector;

Database::Database(const string & _dbname, const string & _user, const string _password) :
    dbname(_dbname), user(_user), password(_password), c(NULL) {
}

connection * Database::connect() {
  connection * c = NULL;
  stringstream ss_connect;
  ss_connect << "dbname=" << this->dbname << " user=" << this->user
             << " password=" << this->password;
  try {
    c = new connection(ss_connect.str());
    if (c->is_open()) {
      std::cout << "Opened database successfully: " << c->dbname() << std::endl;
      this->c = c;
      return c;
    }
    else {
      std::cout << "Can't open database" << std::endl;
      return NULL;
    }
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << std::endl;
    return NULL;
  }
}

void executeSQL(connection * c, const string & sql) {
    try
    {
        work w(*c);
        w.exec(sql);
        w.commit();
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
      std::cout << "fail to execute SQL" << std::endl;
    }
}

void dropTables(connection * c, const vector<string> & tables) {
  string sql = "DROP TABLE IF EXISTS ";
  if (tables.size() > 0) {
    sql.append("\"" + tables[0] + "\"");
    for (size_t i = 1; i < tables.size(); i++) {
      sql.append(", \"" + tables[i] + "\"");
    }
  }
  executeSQL(c, sql);
}

void createAccount(connection * c) {
  string sql = "CREATE TABLE \"account\" (\n"
               "\"account_id\"             SERIAL     NOT NULL,\n"
               "\"balance\"                DECIMAL(20,2)     NOT NULL,\n"
               "CONSTRAINT ACCOUNTPK PRIMARY KEY (\"account_id\"));";
  executeSQL(c, sql);
}

void createPosition(connection * c) {
  string sql = "CREATE TABLE \"position\" (\n"
               "\"symbol\"             VARCHAR(30)     NOT NULL ,\n"
               "\"amount\"             INT     NOT NULL,\n"
               "\"owner_id\"          INT     NOT NULL,\n"
               "CONSTRAINT POSITIONPK PRIMARY KEY (\"symbol\",\"owner_id\"),\n"
               "CONSTRAINT OWNERIDFK FOREIGN KEY (\"owner_id\") REFERENCES "
               "\"account\"(\"account_id\") ON DELETE SET NULL ON UPDATE CASCADE);";
  executeSQL(c, sql);
}

void createOpen(connection * c) {
  string sql = "CREATE TABLE \"open\" (\n"
               "\"trans_id\"             SERIAL     NOT NULL ,\n"
               "\"account_id\"        INT     NOT NULL ,\n"
               "\"symbol\"             VARCHAR(30)     NOT NULL ,\n"
               "\"limit_value\"             DECIMAL(20,2)     NOT NULL ,\n"
               "\"amount\"             INT     NOT NULL,\n"
               "CONSTRAINT TRANSIDPK PRIMARY KEY (\"trans_id\"), \n"
               "CONSTRAINT ACCOUNTIDFK FOREIGN KEY (\"account_id\") REFERENCES "
               "\"account\"(\"account_id\") ON DELETE SET NULL ON UPDATE CASCADE);";
  executeSQL(c, sql);
}

void createExecuted(connection * c) {
  string sql = "CREATE TABLE \"executed\" (\n"
               "\"exect_id\"             SERIAL     NOT NULL ,\n"
               "\"trans_id\"        INT     NOT NULL ,\n"
               "\"amount\"            INT     NOT NULL ,\n"
               "\"price\"             DECIMAL(20,2)     NOT NULL ,\n"
               "\"time\"            INT     NOT NULL ,\n"
               "\"symbol\"             VARCHAR(30)     NOT NULL ,\n"
               "\"seller_id\"             INT     NOT NULL ,\n"
               "\"buyer_id\"             INT     NOT NULL,\n"
               "CONSTRAINT EXECTIDPK PRIMARY KEY (\"exect_id\"), \n"
               "CONSTRAINT TRANSIDFK FOREIGN KEY (\"trans_id\") REFERENCES "
               "\"open\"(\"trans_id\") ON DELETE SET NULL ON UPDATE CASCADE);";
  executeSQL(c, sql);
}

void createCancelled(connection * c) {
  string sql = "CREATE TABLE \"cancelled\" (\n"
               "\"cancel_id\"             SERIAL     NOT NULL ,\n"
               "\"trans_id\"        INT     NOT NULL ,\n"
               "\"amount\"            INT     NOT NULL ,\n"
               "\"time\"            INT     NOT NULL ,\n"
               "\"account_id\"             INT     NOT NULL ,\n"
               "CONSTRAINT CANCELIDPK PRIMARY KEY (\"cancel_id\"), \n"
               "CONSTRAINT TRANSIDFK FOREIGN KEY (\"trans_id\") REFERENCES "
               "\"open\"(\"trans_id\") ON DELETE SET NULL ON UPDATE CASCADE);";
  executeSQL(c, sql);
}

void createTables(connection * c) {
  createAccount(c);
  createPosition(c);
  createOpen(c);
  createExecuted(c);
  createCancelled(c);
}

void Database::initialize() {
  vector<string> tables(5);
  tables[0] = "account";
  tables[1] = "position";
  tables[2] = "open";
  tables[3] = "executed";
  tables[4] = "cancelled";
  dropTables(this->c, tables);
  createTables(this->c);
}

bool Database::insert_account(const string& account_id, const size_t& balance){
    string sql;
    if (find_account(account_id)) return false;
    
    sql = "INSERT INTO account (account_id, balance) ";
    sql += string("VALUES (");
    sql += account_id + ",";
    sql += std::to_string(balance);
    sql += string("); ");
    std::cout << "start to execute sql" << std::endl;
    executeSQL(c, sql);
    std::cout << "finished executing sql" << std::endl;
    return true;
}

bool Database::insert_sym(const string& account_id, const string& sym, const size_t& num){
    string sql = "SELECT amount FROM account, position WHERE account_id=owner_id";
    sql += " AND account_id=" + account_id;
    sql += " AND symbol=" + c->quote(sym) + ";";
    try
    {
        nontransaction N(*c);
        /* Execute SQL query */
        result R( N.exec( sql ));
        if (R.begin() != R.end()){
          // update value
          result::const_iterator c = R.begin();
          std::cout << "UPDATE VALUE IN POSITION" << std::endl;
          int new_amount = c[0].as<int>() + num;
          sql = "UPDATE position ";
          sql += "SET ";
          sql += "amount=" + std::to_string(new_amount);
          sql += " WHERE owner_id=" + account_id + ";";
        } else {
          // insert new value
            sql = "INSERT INTO position (symbol, amount, owner_id) ";
            sql += string("VALUES (");
            sql += c->quote(sym) + ",";
            sql += std::to_string(num) + ",";
            sql += account_id;
            sql += string("); ");

        }
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
    }
    std::cout << sql << std::endl;
    executeSQL(c, sql);
    return true;
}

bool Database::insert_order(const string& account_id, const string& sym, const int& amount, const double& limit, size_t& trans_id){
    string sql;
    try
    {
      // check order request is valid, like enough symbol amount and enough balance
      // TODO: amount == 0
        if (amount > 0){
          // buy order: check enough balance
          std::cout << "check enough balance" << std::endl;
          sql = "SELECT balance FROM account WHERE account_id=" + account_id + ";";
          nontransaction N(*c);
          /* Execute SQL query */
          result R( N.exec( sql ));
          result::const_iterator c = R.begin();
          double account_balance = c[0].as<double>();
          if (account_balance < limit * amount){
            std::cout << "check enough balance" << std::endl;
            return false;
          }
        } else {
          // sell order: check enough symbol amount and valid symbol
          sql = "SELECT amount FROM account, position WHERE account_id=owner_id";
          sql += " AND account_id=" + account_id;
          sql += " AND symbol=" + c->quote(sym) + ";";
          nontransaction N(*c);
          /* Execute SQL query */
          result R( N.exec( sql ));
          if (R.begin() == R.end()){
            // check whether the client has the order
            std::cout << "client does not have the symbol" << std::endl;
            return false;
          }
          result::const_iterator c = R.begin();
          int symbol_amount = c[0].as<int>();
          if (symbol_amount < amount){
            std::cout << "check enough symbol amount" << std::endl;
            return false;
          }
        }
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
      std::cout << "fail to check open order is valid" << std::endl;
    }
    
    try
    {
      // insert the order into database
      std::cout << "insert the order into database" << std::endl;
      sql = "INSERT INTO open (account_id, symbol, limit_value, amount) ";
      sql += string("VALUES (");
      sql += account_id + ",";
      sql += c->quote(sym) + ",";
      sql += std::to_string(limit) + ",";
      sql += std::to_string(amount);
      sql += string("); ");
      executeSQL(c, sql);
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
      std::cout << "fail to insert open order" << std::endl;
    }

    try
    {
      // get the transaction id from open table
      std::cout << "get the transaction id from open table" << std::endl;
      sql = "SELECT trans_id FROM open WHERE";
      sql += " account_id=" + account_id;
      sql += " AND limit_value=" + std::to_string(limit);
      sql += " AND amount=" + std::to_string(amount);
      sql += " AND symbol=" + c->quote(sym) + ";";
      nontransaction N(*c);
      /* Execute SQL query */
      result R( N.exec( sql ));
      result::const_iterator c = R.begin();
      trans_id = c[0].as<int>();
      return true;
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
      std::cout << "fail to get the transaction id" << std::endl;
    }
    return true; 
}

void Database::executed_order(const string& account_id, const string& sym, const int& amount, const double& limit, const size_t& trans_id){
  // TODO: add mutex lock guard
  string sql;
  result R;
  try
  {
    // find all the possible order
    if (amount > 0){
      // buy order
      sql = "SELECT trans_id,account_id,symbol,limit_value,amount FROM open WHERE";
      sql += " symbol=" + c->quote(sym);
      sql += " AND limit_value<=" + std::to_string(limit);
      sql += " AND amount<=0;";
    } else {
      // sell order
      sql = "SELECT trans_id,account_id,symbol,limit_value,amount FROM open WHERE";
      sql += " symbol=" + c->quote(sym);
      sql += " AND limit_value>=" + std::to_string(limit);
      sql += " AND amount>=0;";
    }
      nontransaction N(*c);
      /* Execute SQL query */
      R = N.exec(sql);
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    std::cout << "fail to find valid open order" << std::endl;
  }

  try
  {
    // start to execute all possible open orders
    if (amount > 0){
      // buy order
      int buyer_amount = amount;
      for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
        int seller_trans_id = c[0].as<int>();
        string seller_account_id = to_string(c[1].as<int>());
        assert(sym == c[2].as<string>());
        int seller_amount = c[4].as<int>();
        if (buyer_amount == 0){
          // TODO: erase the open order
          return;
        } else if (buyer_amount > abs(seller_amount)){
            buyer_amount -= abs(seller_amount);
            execute_single_open_order(trans_id,account_id,buyer_amount,seller_trans_id,seller_account_id,0,sym,seller_amount,limit);
        } else {
            seller_amount += buyer_amount;
            execute_single_open_order(trans_id,account_id,0,seller_trans_id,seller_account_id,seller_amount,sym,buyer_amount,limit);
            buyer_amount = 0;
        }
      }
    } else {
      // sell order
      int seller_amount = amount;
      for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
        int buyer_trans_id = c[0].as<int>();
        string buyer_account_id = to_string(c[1].as<int>());
        assert(sym == c[2].as<string>());
        int buyer_limit = c[3].as<double>();
        int buyer_amount = c[4].as<int>();
        if (seller_amount == 0){
          // erase the open order
          return;
        } else if (abs(seller_amount) > buyer_amount){
            std::cout << "executed all buyer_amount and seller amount: " << seller_amount << std::endl;
            seller_amount += buyer_amount;
            execute_single_open_order(buyer_trans_id,buyer_account_id,0,trans_id,account_id,seller_amount,sym,buyer_amount,buyer_limit);
        } else {
            std::cout << "seller amount not enough and seller amount: " << seller_amount << std::endl;
            buyer_amount -= abs(seller_amount);
            execute_single_open_order(buyer_trans_id,buyer_account_id,buyer_amount,trans_id,account_id,0,sym,seller_amount,buyer_limit);
            seller_amount = 0;
        }
      }
    }   
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  
  
}

void Database::execute_single_open_order(const size_t& trans_id, const string& account_id, const int& buyer_amount,\
    const size_t& seller_trans_id, const string& seller_account_id, const int& seller_amount,\
    const string& sym, const int& order_amount, const double& order_limit){
    string sql;
    try
    {
      // update seller and buyer's open order
      std::cout << "UPDATE VALUE IN POSITION" << std::endl;
      // update buyer
      sql = "UPDATE open ";
      sql += "SET ";
      sql += "amount=" + std::to_string(buyer_amount);
      sql += " WHERE trans_id=" + std::to_string(trans_id) + ";";
      executeSQL(c, sql);

      sql = "UPDATE account ";
      sql += "SET ";
      sql += "balance=balance-" + std::to_string(order_amount * order_limit);
      sql += " WHERE account_id=" + account_id + ";";
      executeSQL(c, sql);

      // update seller
      sql = "UPDATE open ";
      sql += "SET ";
      sql += "amount=" + std::to_string(seller_amount);
      sql += " WHERE trans_id=" + std::to_string(seller_trans_id) + ";";
      executeSQL(c, sql);
      sql = "UPDATE account ";
      sql += "SET ";
      sql += "balance=balance+" + std::to_string(order_amount * order_limit);
      sql += " WHERE account_id=" + seller_account_id + ";";
      executeSQL(c, sql);
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
      std::cout << "fail to update" << std::endl;
    }

    try
    {
      // insert order into executed table
      // insert buyer
      sql = "INSERT INTO executed (trans_id,amount,price,time,symbol,seller_id,buyer_id) ";
      sql += string("VALUES (");
      sql += std::to_string(trans_id) + ",";
      sql += std::to_string(order_amount) + ",";
      sql += std::to_string(order_limit) + ",";
      sql += std::to_string(0) + ",";
      sql += c->quote(sym) + ",";
      sql += seller_account_id + ",";
      sql += account_id;
      sql += string("); ");
      executeSQL(c, sql);

      // insert seller
      sql = "INSERT INTO executed (trans_id,amount,price,time,symbol,seller_id,buyer_id) ";
      sql += string("VALUES (");
      sql += std::to_string(seller_trans_id) + ",";
      sql += std::to_string(order_amount) + ",";
      sql += std::to_string(order_limit) + ",";
      sql += std::to_string(0) + ",";
      sql += c->quote(sym) + ",";
      sql += seller_account_id + ",";
      sql += account_id;
      sql += string("); ");
      executeSQL(c, sql);
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
    }
}

bool Database::find_account(const string& account_id){
  string sql = "SELECT account_id FROM account WHERE account_id=" + account_id + ";";
  nontransaction N(*c);
  /* Execute SQL query */
  result R( N.exec( sql ));
  if (R.begin() != R.end()){
    // account already exists
    return true;
  }
  return false;
}

// Database::~Database() {
//   if (this->c != NULL) {
//     c->disconnect();
//     delete this->c;
//   }
// }
