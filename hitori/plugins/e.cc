#include "e.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

LibE::LibE() : stuff_(new int(42)) {}

LibE::~LibE() { delete stuff_; }

void LibE::Use() {
  srand(*stuff_);
  *stuff_ = 1;
}

}  // namespace hitori::plugins::helpers
