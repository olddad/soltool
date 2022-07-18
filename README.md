tools to test pub/sub for solace using flex/bison

##### setup LD_LIBRARY_PATH
Need solace client lib to run the tools. Can download from http://dev.solace.com/tech/c-api/

```
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$(SOLCLIENTDIR}/lib

e.g.
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${SOLCLIENTDIR}/lib
```

##### send msg:
```
cat msg.dat | ./solsend -s localhost -n default -u admin -p <passwd>
```

##### listen:
```
./sollisten -s localhost -n default -u admin -p <passwd> TEST

using kerberos
./sollisten -s localhost -n default -u admin -k "TEST/>"
```

##### msg example:
see msg.dat

##### others
