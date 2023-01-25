.DELETE_ON_ERROR:

CXX=g++
CXXFLAGS=-O2 -Wall -std=c++20 -fstack-protector-strong -mavx2 -march=skylake
HEADERS=speck.h
IMAGE_NAME=ghcr.io/riatre/aliyunctf-lyla
IMAGE_TAG=latest

all: lyla image

image: deploy/image.tar.zst

lyla: lyla.clean inject_backdoor.py check_inject_fini.s backdoor.s
	python3 inject_backdoor.py
	chmod +x lyla

lyla.clean.unstripped: main.o speck.o
	$(CXX) $(CXXFLAGS) -o$@ $(filter %.o, $^) -Wl,-z,now

# We have to strip the file before doing funny things to the ELF since strip breaks our ELF
# TODO(riatre): ... address this, as stripping the (already stripped) final ELF also breaks it, this may tip off players.
#               Reason: strip erases end-of-segment extra data :(
lyla.clean: lyla.clean.unstripped
	strip -o lyla.clean lyla.clean.unstripped

test: speck_test chall_test image_test
	./speck_test
	./test-chall.sh
	./test-image.sh "$(IMAGE_NAME):$(IMAGE_TAG)"

chall_test: lyla solve.py flag.txt

speck_test: speck.o speck_test.o
	$(CXX) $(CXXFLAGS) -o$@ $^ -lgtest_main -lgtest

image_test: test-image.sh image.tar.zst

deploy/image.tar.zst: deploy/Dockerfile lyla
	docker buildx build -f deploy/Dockerfile --output type=docker,dest=- -t $(IMAGE_NAME):$(IMAGE_TAG) . | zstd -c > deploy/image.tar.zst

%.o: %.cc $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $<

clean:
	rm -f *.o lyla lyla.clean lyla.clean.unstripped speck_test deploy/image.tar.zst

.PHONY: test clean
