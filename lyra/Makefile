CXX=g++
CXXFLAGS=-O2 -Wall -std=c++20 -fstack-protector-strong -mavx2 -march=skylake
HEADERS=speck.h

lyra: lyra.clean inject_backdoor.py check_inject_fini.s backdoor.s
	python3 inject_backdoor.py
	chmod +x lyra

lyra.clean.unstripped: main.o speck.o
	$(CXX) $(CXXFLAGS) -o$@ $(filter %.o, $^) -Wl,-z,now

# We have to strip the file before doing funny things to the ELF since strip breaks our ELF
# TODO(riatre): ... address this, as stripping the (already stripped) final ELF also breaks it, this may tip off players.
#               Reason: strip erases end-of-segment extra data :(
lyra.clean: lyra.clean.unstripped
	strip -o lyra.clean lyra.clean.unstripped

test: speck_test chall_test
	./speck_test
	./test-chall.sh

chall_test: lyra solve.py flag.txt

speck_test: speck.o speck_test.o
	$(CXX) $(CXXFLAGS) -o$@ $^ -lgtest_main -lgtest

%.o: %.cc $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $<

clean:
	rm -f *.o lyra lyra.clean lyra.clean.unstripped speck_test

.PHONY: test clean
