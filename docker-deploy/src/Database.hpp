#include <assert.h>

#include <iostream>
#include <pqxx/pqxx>
#include <set>
#include <string>
#include <utility>

// using pqxx::connection;
using namespace pqxx;
using std::pair;
using std::set;
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
  bool insert_account(const string & account_id, const size_t & balance);
  bool insert_sym(const string & account_id, const string & sym, const size_t & num);
  bool insert_order(const string & account_id,
                    const string & sym,
                    const int & amount,
                    const double & limit,
                    size_t & trans_id);
  void insert_cancel(const int trans_id,
                     const int amount,
                     const int time,
                     const int account_id);

  // execute order
  void executed_order(const string & account_id,
                      const string & sym,
                      const int & amount,
                      const double & limit,
                      const size_t & trans_id);
  void execute_single_open_order(const size_t & trans_id,
                                 const string & account_id,
                                 const int & buyer_amount,
                                 const size_t & seller_trans_id,
                                 const string & seller_account_id,
                                 const int & seller_amount,
                                 const string & sym,
                                 const int & order_amount,
                                 const double & order_limit,
                                 const double & pre_limit);
  // delete order
  void delete_single_open_order(const size_t & trans_id);

  // query
  bool find_account(const string & account_id);
  bool has_trans(const int trans_id,
                 const int account_id,
                 const string & account_id_name,
                 const string & table);
  bool is_own_trans(const int trans_id, const int account_id);
  void set_open_result(const int trans_id, int & shares);
  void set_cancelled_result(const int trans_id, set<pair<int, int> > & cancelled_set);
  void set_executed_result(const int trans_id,
                           set<pair<int, pair<double, int> > > & executed_set);
  // cancel
  void return_balance(const int account_id, const double balance);
  void return_symbol(const int account_id, const string & symbol, const int amount);
  void cancel_transaction(const size_t trans_id);
  // ~Database();


  // disconnect
  void disconnect();
};

#endif  //_QUERY_FUNCS_
