# NOTE: Can't target an ancient libc. TEXTREL with IFUNC needs glibc>=2.29.
# TODO: Build with -static-libstdc++? The code looks too ugly. 
# Need to find a better way to distract players from unusual relocation entries.
# We have to use unsuspecting reloc (or encoding) for IDA Pro evasion anyways, so it's okay.

CXX=g++
CXXFLAGS=-O2 -Wall -std=c++20
HEADERS=speck.h

lyra.clean: main.o speck.o ldscript
	$(CXX) $(CXXFLAGS) -o$@ $(filter %.o, $^) -Wl,-z,now -Wl,--defsym,Ri=0 -Wl,--export-dynamic-symbol,Ri -T ldscript

test: speck_test
	./speck_test

speck_test: speck.o speck_test.o
	$(CXX) $(CXXFLAGS) -o$@ $^ -lgtest_main -lgtest

%.o: %.cc $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $<

clean:
	rm *.o lyra lyra.clean speck_test

.PHONY: test clean
