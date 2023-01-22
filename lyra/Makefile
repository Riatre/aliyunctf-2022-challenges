# NOTE: Can't target an ancient libc. TEXTREL with IFUNC needs glibc>=2.29.
# TODO: Build with -static-libstdc++? The code looks too ugly. 
# Need to find a better way to distract players from unusual relocation entries.
# We have to use unsuspecting reloc (or encoding) for IDA Pro evasion anyways, so it's okay.

CXX=g++
CXXFLAGS=-O2 -Wall -std=c++20 -fstack-protector-strong
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

test: speck_test
	./speck_test

speck_test: speck.o speck_test.o
	$(CXX) $(CXXFLAGS) -o$@ $^ -lgtest_main -lgtest

%.o: %.cc $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $<

clean:
	rm -f *.o lyra lyra.clean speck_test

.PHONY: test clean
