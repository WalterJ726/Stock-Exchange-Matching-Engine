#include <iostream>
#include <pqxx/pqxx>
#include <string>
#include <assert.h>

// using pqxx::connection;
using namespace pqxx;
using std::string;

#ifndef _QUERY_FUNCS_
#define _QUERY_FUNCS_

class Database {
 private:
  const string dbname;
  const string user;
  const string password;
  connection * c;

 public:
  Database(const string & dbname, const string & user, const string password);

  //Establish a connection to the database
  //Parameters: database name, user name, user password
  connection * connect();

  //drop tables if already exist, and create tables
  void initialize();

  // insert value to database
  bool insert_account(const string& account_id, const size_t& balance);
  bool insert_sym(const string& account_id, const string& sym, const size_t& num);
  bool insert_order(const string& account_id, const string& sym, const int& amount, const double& limit, size_t& trans_id);

  // execute order
  void executed_order(const string& account_id, const string& sym, const int& amount, const double& limit, const size_t& trans_id);
  void execute_single_open_order(const size_t& trans_id, const string& account_id, const int& buyer_amount,\
                                const size_t& seller_trans_id, const string& seller_account_id, const int& seller_amount,\
                                const string& sym, const int& order_amount, const double& order_limit);
  // query
  bool find_account(const string& account_id);
  // ~Database();
};

#endif  //_QUERY_FUNCS_
