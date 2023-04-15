PARSER=parser
SSVR=search_server
HTTP_SERVER=http_server
cc=g++

.PHONY:all
all:$(PARSER) $(SSVR) $(HTTP_SERVER)
$(PARSER):parser.cc
	$(cc) -o $@ $^ -lboost_system -lboost_filesystem -std=c++11

$(SSVR): server.cc	
	$(cc) -o $@ $^  -ljsoncpp -std=c++11

$(HTTP_SERVER): http_server.cc
	$(cc) -o $@ $^  -ljsoncpp -lpthread -std=c++11

.PHONY:clean
clean:
	rm -rf $(PARSER) $(SSVR) $(HTTP_SERVER)
