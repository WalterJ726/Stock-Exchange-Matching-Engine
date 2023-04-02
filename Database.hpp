#include <iostream>
#include <pqxx/pqxx>
#include <string>

using pqxx::connection;
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

  // ~Database();
};

#endif  //_QUERY_FUNCS_
