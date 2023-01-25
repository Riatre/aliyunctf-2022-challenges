.DELETE_ON_ERROR:

CXX=g++
CXXFLAGS=-O2 -Wall -std=c++20 -fstack-protector-strong -mavx2 -march=skylake
HEADERS=speck.h
IMAGE_NAME=ghcr.io/riatre/aliyunctf-lyra
IMAGE_TAG=latest

all: lyra image.tar.zst

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

test: speck_test chall_test image_test
	./speck_test
	./test-chall.sh
	./test-image.sh "$(IMAGE_NAME):$(IMAGE_TAG)"

chall_test: lyra solve.py flag.txt

speck_test: speck.o speck_test.o
	$(CXX) $(CXXFLAGS) -o$@ $^ -lgtest_main -lgtest

image_test: test-image.sh image.tar.zst

image.tar.zst: Dockerfile.deploy lyra
	docker buildx build -f Dockerfile.deploy --output type=docker,dest=- -t $(IMAGE_NAME):$(IMAGE_TAG) . | zstd -c > image.tar.zst

%.o: %.cc $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $<

clean:
	rm -f *.o lyra lyra.clean lyra.clean.unstripped speck_test

.PHONY: test clean
