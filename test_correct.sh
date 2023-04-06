HOSTNAME=localhost

./testHandleRequest $HOSTNAME 12345 testcase/add_acount.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions1.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions2.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions3.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions4.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions5.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions6.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions7.xml
./testHandleRequest $HOSTNAME 12345 testcase/transactions_query1.xml
#./testHandleRequest $HOSTNAME 12345 testcase/my_add_account.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/my_trans_1.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/my_trans_2.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/my_trans_3.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/my_trans_4.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/my_trans_5.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/error1//my_add_account.xml
#./testHandleRequest $HOSTNAME 12345 testcase/error1/my_trans_1.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/error1/my_trans_2.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/error1/my_trans_3.xml 
#./testHandleRequest $HOSTNAME 12345 testcase/error1/my_trans_4.xml 

