#include "f.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

LibF::LibF() : stuff_(new int(42)) {}

LibF::~LibF() { delete stuff_; }

void LibF::Use() {
  srand(*stuff_);
  *stuff_ = 1;
}

}  // namespace hitori::plugins::helpers
