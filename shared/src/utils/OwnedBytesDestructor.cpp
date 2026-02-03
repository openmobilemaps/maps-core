#include "OwnedBytesDestructor.h"
#include "OwnedBytes.h"

void OwnedBytesDestructor::free(const OwnedBytes & bytes) {
    ::operator delete(reinterpret_cast<void*>(bytes.address));
}
