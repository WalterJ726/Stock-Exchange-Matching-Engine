
  string sql = "CREATE TABLE \"account\" (\n"
               "\"account_id\"             SERIAL     NOT NULL,\n"
               "\"balance\"                DECIMAL(20,2)     NOT NULL,\n"
               "CONSTRAINT ACCOUNTPK PRIMARY KEY (\"account_id\"));";


  string sql = "CREATE TABLE \"position\" (\n"
               "\"symbol\"             VARCHAR(30)     NOT NULL ,\n"
               "\"amount\"             INT     NOT NULL,\n"
               "\"owner_id\"          INT     NOT NULL,\n"
               "CONSTRAINT POSITIONPK PRIMARY KEY (\"symbol\",\"owner_id\"),\n"
               "CONSTRAINT OWNERIDFK FOREIGN KEY (\"owner_id\") REFERENCES "
               "\"account\"(\"account_id\") ON DELETE SET NULL ON UPDATE CASCADE);";

  string sql = "CREATE TABLE \"open\" (\n"
               "\"trans_id\"             SERIAL     NOT NULL ,\n"
               "\"account_id\"        INT     NOT NULL ,\n"
               "\"symbol\"             VARCHAR(30)     NOT NULL ,\n"
               "\"limit_value\"             DECIMAL(20,2)     NOT NULL ,\n"
               "\"amount\"             INT     NOT NULL,\n"
               "CONSTRAINT TRANSIDPK PRIMARY KEY (\"trans_id\"), \n"
               "CONSTRAINT ACCOUNTIDFK FOREIGN KEY (\"account_id\") REFERENCES "
               "\"account\"(\"account_id\") ON DELETE SET NULL ON UPDATE CASCADE);";


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


  string sql = "CREATE TABLE \"cancelled\" (\n"
               "\"cancel_id\"             SERIAL     NOT NULL ,\n"
               "\"trans_id\"        INT     NOT NULL ,\n"
               "\"amount\"            INT     NOT NULL ,\n"
               "\"time\"            INT     NOT NULL ,\n"
               "\"account_id\"             INT     NOT NULL ,\n"
               "CONSTRAINT CANCELIDPK PRIMARY KEY (\"cancel_id\"), \n"
               "CONSTRAINT TRANSIDFK FOREIGN KEY (\"trans_id\") REFERENCES "
               "\"open\"(\"trans_id\") ON DELETE SET NULL ON UPDATE CASCADE);";