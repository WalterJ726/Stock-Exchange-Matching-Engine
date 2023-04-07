#include "Database.hpp"

#include <ctime>
#include <sstream>
#include <vector>
using std::stringstream;
using std::vector;

Database::Database(const string & _dbname, const string & _user, const string _password) :
    dbname(_dbname), user(_user), password(_password), c(NULL) {
}

connection * Database::connect() {
  connection * c = NULL;
  stringstream ss_connect;
  ss_connect << "host=db port=5432 dbname=" << this->dbname << " user=" << this->user
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

result executeSQL(connection * c, const string & sql) {
  try {
    work w(*c);
    // w.exec("set transaction isolation level serializable");
    result r(w.exec(sql));
    w.commit();
    return r;
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to execute SQL" << std::endl;
  }
  return result();  // if an error occurs, return an empty result
}

int getCurrTime() {
  // get the current time in seconds since the epoch
  std::time_t now = std::time(nullptr);

  // save the time as an integer
  int time_as_int = static_cast<int>(now);

  return time_as_int;
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
               "\"account_id\"            INT     NOT NULL,\n"
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
               "CONSTRAINT EXECTIDPK PRIMARY KEY (\"exect_id\"));";
  executeSQL(c, sql);
}

void createCancelled(connection * c) {
  string sql = "CREATE TABLE \"cancelled\" (\n"
               "\"cancel_id\"             SERIAL     NOT NULL ,\n"
               "\"trans_id\"        INT     NOT NULL ,\n"
               "\"amount\"            INT     NOT NULL ,\n"
               "\"time\"            INT     NOT NULL ,\n"
               "\"account_id\"             INT     NOT NULL ,\n"
               "CONSTRAINT CANCELIDPK PRIMARY KEY (\"cancel_id\"));";
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

bool Database::insert_account(const string & account_id, const size_t & balance) {
  string sql;
  if (find_account(account_id))
    return false;

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

bool Database::insert_sym(const string & account_id,
                          const string & sym,
                          const size_t & num) {
  string sql = "SELECT amount FROM account, position WHERE account_id=owner_id";
  sql += " AND account_id=" + account_id;
  sql += " AND symbol=" + c->quote(sym) + ";";
  try {
    nontransaction N(*c);
    /* Execute SQL query */
    result R(N.exec(sql));
    if (R.begin() != R.end()) {
      // update value
      result::const_iterator c = R.begin();
      std::cout << "UPDATE VALUE IN POSITION" << std::endl;
      int new_amount = c[0].as<int>() + num;
      sql = "UPDATE position ";
      sql += "SET ";
      sql += "amount=" + std::to_string(new_amount);
      sql += " WHERE owner_id=" + account_id + ";";
    }
    else {
      // insert new value
      sql = "INSERT INTO position (symbol, amount, owner_id) ";
      sql += string("VALUES (");
      sql += c->quote(sym) + ",";
      sql += std::to_string(num) + ",";
      sql += account_id;
      sql += string("); ");
    }
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
  }
  std::cout << sql << std::endl;
  executeSQL(c, sql);
  return true;
}

bool Database::insert_order(const string & account_id,
                            const string & sym,
                            const int & amount,
                            const double & limit,
                            size_t & trans_id) {
  string sql;
  // check order request is valid, like enough symbol amount and enough balance
  // TODO: amount == 0
  if (amount > 0) {
    // buy order: check enough balance
    std::cout << "check enough balance" << std::endl;
    sql = "SELECT balance FROM account WHERE account_id=" + account_id + ";";
    try {
      nontransaction N(*c);
      /* Execute SQL query */
      result R(N.exec(sql));
      result::const_iterator c = R.begin();
      double account_balance = c[0].as<double>();
      if (account_balance < limit * amount) {
        std::cout << "check enough balance" << std::endl;
        return false;
      }
    }
    catch (const std::exception & e) {
      std::cerr << e.what() << '\n';
      std::cout << "fail to check open order is valid" << std::endl;
    }
    // deduct money from buyer account
    sql = "UPDATE account ";
    sql += "SET ";
    sql += "balance=balance-" + std::to_string(amount * limit);
    sql += " WHERE account_id=" + account_id + ";";
    executeSQL(c, sql);
  }
  else {
    // sell order: check enough symbol amount and valid symbol
    sql = "SELECT amount FROM account, position WHERE account_id=owner_id";
    sql += " AND account_id=" + account_id;
    sql += " AND symbol=" + c->quote(sym) + ";";

    try {
      nontransaction N(*c);
      /* Execute SQL query */
      result R(N.exec(sql));
      if (R.begin() == R.end()) {
        // check whether the client has the order
        std::cout << "client does not have the symbol" << std::endl;
        return false;
      }
      result::const_iterator c = R.begin();
      int symbol_amount = c[0].as<int>();
      if (symbol_amount < abs(amount)) {
        std::cout << "check enough symbol amount" << std::endl;
        return false;
      }
    }
    catch (const std::exception & e) {
      std::cerr << e.what() << '\n';
    }
    // deducted shares from seller
    sql = "UPDATE position ";
    sql += "SET ";
    sql += "amount=amount-" + std::to_string(abs(amount));
    sql += " WHERE owner_id=" + account_id;
    sql += " AND symbol=" + c->quote(sym) + ";";
    executeSQL(c, sql);
  }

  try {
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
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to insert open order" << std::endl;
  }

  try {
    // get the transaction id from open table
    std::cout << "get the transaction id from open table" << std::endl;
    sql = "SELECT trans_id FROM open WHERE";
    sql += " account_id=" + account_id;
    sql += " AND limit_value=" + std::to_string(limit);
    sql += " AND amount=" + std::to_string(amount);
    sql += " AND symbol=" + c->quote(sym) + ";";
    nontransaction N(*c);
    /* Execute SQL query */
    result R(N.exec(sql));
    result::const_iterator c = R.begin();
    trans_id = c[0].as<int>();
    return true;
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to get the transaction id" << std::endl;
  }
  return true;
}

void Database::insert_cancel(const int trans_id,
                             const int amount,
                             const int time,
                             const int account_id) {
  try {
    stringstream ss_sql;
    ss_sql << "insert into cancelled(trans_id, amount, time, account_id) values("
           << trans_id << ", " << amount << ", " << time << ", " << account_id << ");";
    executeSQL(c, ss_sql.str());
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to insert into table cancelled" << std::endl;
  }
}

void Database::executed_order(const string & account_id,
                              const string & sym,
                              const int & amount,
                              const double & limit,
                              const size_t & trans_id) {
  // TODO: add mutex lock guard
  string sql;
  result R;
  try {
    // find all the possible order
    if (amount > 0) {
      // buy order
      sql = "SELECT trans_id,account_id,symbol,limit_value,amount FROM open WHERE";
      sql += " symbol=" + c->quote(sym);
      sql += " AND limit_value<=" + std::to_string(limit);
      sql += " AND amount<=0 ORDER BY limit_value ASC;";
    }
    else {
      // sell order
      sql = "SELECT trans_id,account_id,symbol,limit_value,amount FROM open WHERE";
      sql += " symbol=" + c->quote(sym);
      sql += " AND limit_value>=" + std::to_string(limit);
      sql += " AND amount>=0 ORDER BY limit_value DESC;";
    }
    nontransaction N(*c);
    /* Execute SQL query */
    R = N.exec(sql);
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to find valid open order" << std::endl;
  }

  try {
    // start to execute all possible open orders
    if (amount > 0) {
      // buy order
      int buyer_amount = amount;
      for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
        int seller_trans_id = c[0].as<int>();
        string seller_account_id = to_string(c[1].as<int>());
        assert(sym == c[2].as<string>());
        double seller_limit = c[3].as<double>();
        int seller_amount = c[4].as<int>();
        if (buyer_amount == 0) {
          return;
        }
        else if (buyer_amount > abs(seller_amount)) {
          buyer_amount -= abs(seller_amount);
          execute_single_open_order(trans_id,
                                    account_id,
                                    buyer_amount,
                                    seller_trans_id,
                                    seller_account_id,
                                    0,
                                    sym,
                                    abs(seller_amount),
                                    seller_limit,
                                    limit);
        }
        else {
          seller_amount += buyer_amount;
          execute_single_open_order(trans_id,
                                    account_id,
                                    0,
                                    seller_trans_id,
                                    seller_account_id,
                                    seller_amount,
                                    sym,
                                    buyer_amount,
                                    seller_limit,
                                    limit);
          buyer_amount = 0;
        }
      }
    }
    else {
      // sell order
      int seller_amount = amount;
      for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
        int buyer_trans_id = c[0].as<int>();
        string buyer_account_id = to_string(c[1].as<int>());
        assert(sym == c[2].as<string>());
        int buyer_limit = c[3].as<double>();
        int buyer_amount = c[4].as<int>();
        if (seller_amount == 0) {
          return;
        }
        else if (abs(seller_amount) > buyer_amount) {
          std::cout << "executed all buyer_amount and seller amount: " << seller_amount
                    << std::endl;
          seller_amount += buyer_amount;
          execute_single_open_order(buyer_trans_id,
                                    buyer_account_id,
                                    0,
                                    trans_id,
                                    account_id,
                                    seller_amount,
                                    sym,
                                    buyer_amount,
                                    buyer_limit,
                                    buyer_limit);
        }
        else {
          std::cout << "seller amount not enough and seller amount: " << seller_amount
                    << std::endl;
          buyer_amount -= abs(seller_amount);
          execute_single_open_order(buyer_trans_id,
                                    buyer_account_id,
                                    buyer_amount,
                                    trans_id,
                                    account_id,
                                    0,
                                    sym,
                                    abs(seller_amount),
                                    buyer_limit,
                                    buyer_limit);
          seller_amount = 0;
        }
      }
    }
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
  }
}

void Database::execute_single_open_order(const size_t & trans_id,
                                         const string & account_id,
                                         const int & buyer_amount,
                                         const size_t & seller_trans_id,
                                         const string & seller_account_id,
                                         const int & seller_amount,
                                         const string & sym,
                                         const int & order_amount,
                                         const double & order_limit,
                                         const double & pre_limit) {
  string sql;
  try {
    // update seller and buyer's open order
    std::cout << "UPDATE VALUE IN POSITION" << std::endl;
    // update buyer
    if (buyer_amount == 0) {
      // delete open order
      delete_single_open_order(trans_id);
    }
    else {
      sql = "UPDATE open ";
      sql += "SET ";
      sql += "amount=" + std::to_string(buyer_amount);
      sql += " WHERE trans_id=" + std::to_string(trans_id) + ";";
      executeSQL(c, sql);
    }

    sql = "UPDATE account ";
    sql += "SET ";
    sql +=
        "balance=balance+" + std::to_string(order_amount * abs(order_limit - pre_limit));
    sql += " WHERE account_id=" + account_id + ";";
    executeSQL(c, sql);

    // update seller
    if (seller_amount == 0) {
      // delete open order
      delete_single_open_order(seller_trans_id);
    }
    else {
      sql = "UPDATE open ";
      sql += "SET ";
      sql += "amount=" + std::to_string(seller_amount);
      sql += " WHERE trans_id=" + std::to_string(seller_trans_id) + ";";
      executeSQL(c, sql);
    }

    sql = "UPDATE account ";
    sql += "SET ";
    sql += "balance=balance+" + std::to_string(order_amount * order_limit);
    sql += " WHERE account_id=" + seller_account_id + ";";
    executeSQL(c, sql);
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to update" << std::endl;
  }

  try {
    // insert order into executed table
    // insert buyer
    sql = "INSERT INTO executed (trans_id,amount,price,time,symbol,seller_id,buyer_id) ";
    sql += string("VALUES (");
    sql += std::to_string(trans_id) + ",";
    sql += std::to_string(order_amount) + ",";
    sql += std::to_string(order_limit) + ",";
    sql += std::to_string((long)time(NULL)) + ",";
    sql += c->quote(sym) + ",";
    sql += seller_account_id + ",";
    sql += account_id;
    sql += string("); ");
    executeSQL(c, sql);

    // insert seller
    sql = "INSERT INTO executed (trans_id,amount,price,time,symbol,seller_id,buyer_id) ";
    sql += string("VALUES (");
    sql += std::to_string(seller_trans_id) + ",";
    sql += std::to_string(-order_amount) + ",";
    sql += std::to_string(order_limit) + ",";
    sql += std::to_string((long)time(NULL)) + ",";
    sql += c->quote(sym) + ",";
    sql += seller_account_id + ",";
    sql += account_id;
    sql += string("); ");
    executeSQL(c, sql);
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
  }
}

void Database::delete_single_open_order(const size_t & trans_id) {
  string sql;
  sql = "DELETE FROM open WHERE trans_id=" + std::to_string(trans_id) + ";";
  executeSQL(c, sql);
}

bool Database::find_account(const string & account_id) {
  string sql = "SELECT account_id FROM account WHERE account_id=" + account_id + ";";
  nontransaction N(*c);
  /* Execute SQL query */
  result R(N.exec(sql));
  if (R.begin() != R.end()) {
    // account already exists
    return true;
  }
  return false;
}

bool Database::has_trans(const int trans_id,
                         const int account_id,
                         const string & account_id_name,
                         const string & table) {
  if (std::strcmp(table.c_str(), "executed") == 0) {
    stringstream ss_sql_seller;
    ss_sql_seller << "select trans_id, amount from executed where " << account_id_name
                  << " = " << account_id << ";";
    result trans_amout = executeSQL(c, ss_sql_seller.str());
    for (result::const_iterator it = trans_amout.begin(); it != trans_amout.end(); ++it) {
      if (std::strcmp(account_id_name.c_str(), "seller_id") == 0) {
        if (trans_id == it[0].as<int>() && it[1].as<int>() < 0) {
          return true;
        }
      }
      if (std::strcoll(account_id_name.c_str(), "buyer_id") == 0) {
        if (trans_id == it[0].as<int>() && it[1].as<int>() > 0) {
          return true;
        }
      }
    }
    return false;
  }
  else {
    stringstream ss_sql;
    ss_sql << "select trans_id from " << table << " where " << account_id_name << " = "
           << account_id << ";";
    result all_trans_id = executeSQL(c, ss_sql.str());
    for (result::const_iterator it = all_trans_id.begin(); it != all_trans_id.end();
         ++it) {
      if (trans_id == it[0].as<int>()) {
        return true;
      }
    }
    return false;
  }
}

bool Database::is_own_trans(const int trans_id, const int account_id) {
  bool in_open = has_trans(trans_id, account_id, "account_id", "open");
  bool in_cancelled = has_trans(trans_id, account_id, "account_id", "cancelled");
  bool in_executed_seller = has_trans(trans_id, account_id, "seller_id", "executed");
  bool in_executed_buyer = has_trans(trans_id, account_id, "buyer_id", "executed");
  return in_open || in_cancelled || in_executed_seller || in_executed_buyer;
}

void Database::set_open_result(const int trans_id, int & shares) {
  stringstream ss_sql;
  ss_sql << "select amount from open where trans_id  = " << trans_id << ";";
  result amount = executeSQL(c, ss_sql.str());
  if (amount.begin() != amount.end()) {
    shares = amount.begin()[0].as<int>();
  }
  else {
    shares = 0;
  }
}

void Database::set_cancelled_result(const int trans_id,
                                    set<pair<int, int> > & cancelled_set) {
  stringstream ss_sql;
  ss_sql << "select amount, time from cancelled where trans_id  = " << trans_id << ";";
  result amount_time = executeSQL(c, ss_sql.str());
  for (result::const_iterator it = amount_time.begin(); it != amount_time.end(); ++it) {
    int amount = it[0].as<int>();
    int time = it[1].as<int>();
    pair<int, int> one_cancelled(amount, time);
    cancelled_set.insert(one_cancelled);
  }
}

void Database::set_executed_result(const int trans_id,
                                   set<pair<int, pair<double, int> > > & executed_set) {
  stringstream ss_sql;
  ss_sql << "select amount, price, time from executed where trans_id  = " << trans_id
         << ";";
  result amount_price_time = executeSQL(c, ss_sql.str());
  for (result::const_iterator it = amount_price_time.begin();
       it != amount_price_time.end();
       ++it) {
    int amount = it[0].as<int>();
    double price = it[1].as<double>();
    int time = it[2].as<int>();
    pair<int, pair<double, int> > one_executed(amount, pair<double, int>(price, time));
    executed_set.insert(one_executed);
  }
}

void Database::return_balance(const int account_id, const double balance) {
  try {
    stringstream ss_sql;
    ss_sql << "update account set balance = balance + " << balance
           << " where account_id = " << account_id << ";";
    executeSQL(c, ss_sql.str());
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to return balance to buyer who try to cancel" << std::endl;
  }
}

void Database::return_symbol(const int account_id,
                             const string & symbol,
                             const int amount) {
  try {
    stringstream ss_sql;
    ss_sql << "update position set amount = amount + " << amount
           << " where owner_id =  " << account_id << "and symbol = \'" << c->esc(symbol)
           << "\';";
    executeSQL(c, ss_sql.str());
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to return symbol amount to seller who try to cancel" << std::endl;
  }
}

void Database::cancel_transaction(const size_t trans_id) {
  try {
    stringstream ss_sql;
    ss_sql << "select account_id, symbol, limit_value, amount from open where trans_id = "
           << trans_id << ";";
    result r = executeSQL(c, ss_sql.str());
    if (r.begin() != r.end()) {
      const int account_id = r.begin()[0].as<int>();
      const string symbol = r.begin()[1].as<string>();
      const double limit_value = r.begin()[2].as<double>();
      const int amount = r.begin()[3].as<int>();
      if (amount == 0) {
        delete_single_open_order(trans_id);
        insert_cancel(trans_id, amount, getCurrTime(), account_id);
      }
      if (amount < 0) {
        delete_single_open_order(trans_id);
        return_symbol(account_id, symbol, std::abs(amount));
        insert_cancel(trans_id, amount, getCurrTime(), account_id);
      }
      if (amount > 0) {
        delete_single_open_order(trans_id);
        return_balance(account_id, amount * limit_value);
        insert_cancel(trans_id, amount, getCurrTime(), account_id);
      }
    }
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    std::cout << "fail to cancel transaction: trans_id = " << trans_id << std::endl;
  }
}

void Database::disconnect() {
  if (this->c != NULL) {
    c->disconnect();
    delete this->c;
  }
}

// Database::~Database() {
//   if (this->c != NULL) {
//     c->disconnect();
//     delete this->c;
//   }
// }
