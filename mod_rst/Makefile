BASE=/usr/local/freeswitch

SOURCES = mod_rst.c  

OBJS = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
 
#-lnlsCppOpu

CXXFLAGS = -fpermissive

%.o: %.c
<<<<<<< HEAD
	g++ -I${BASE}/include -g -I./ ${CXXFLAGS}    -fPIC -c $< -o $@  

%.o: %.cpp
	g++ -I${BASE}/include -g     -fPIC -c $< -o $@ ${CXXFLAGS} -I./  
=======
	g++ -I${BASE}/include/freeswitch -g -I./ ${CXXFLAGS}    -fPIC -c $< -o $@  

%.o: %.cpp
	g++ -I${BASE}/include/freeswitch -g     -fPIC -c $< -o $@ ${CXXFLAGS} -I./  
>>>>>>> dbf8d11635c30ed32030bd6ca41695a0b18d346a


mod_rst.so: $(OBJS)
	g++ -L${BASE}/lib/    -lfreeswitch -g -L./ -lm -shared -fPIC -o mod_rst.so $(OBJS) -lstdc++  


	
clean : 
	rm -rf $(OBJS) mod_rst.so
