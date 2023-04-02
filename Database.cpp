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
  work w(*c);
  w.exec(sql);
  w.commit();
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
  string sql = "SELECT account_id FROM account WHERE account_id=" + account_id + ";";
  std::cout << sql << std::endl;
  try
  {
      nontransaction N(*c);
      /* Execute SQL query */
      result R( N.exec( sql ));
      if (R.begin() != R.end()){
        // account already exists
        return false;
      }
      // add try block to destruct nontransaction
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
    
    sql = "INSERT INTO account (account_id, balance) ";
    sql += string("VALUES (");
    sql += account_id + ",";
    sql += std::to_string(balance);
    sql += string("); ");
    std::cout << "start to execute sql" << std::endl;
    try
    {
      executeSQL(c, sql);
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
    }
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
          sql += string(";");
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
    try
    {
      executeSQL(c, sql);
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << '\n';
    }
    return true;
}
// Database::~Database() {
//   if (this->c != NULL) {
//     c->disconnect();
//     delete this->c;
//   }
// }
