#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sendfile.h>

int main() {
  int fd = open("/flag_inside/flag", O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }
  sendfile(1, fd, NULL, 0x1000);
  return 0;
}