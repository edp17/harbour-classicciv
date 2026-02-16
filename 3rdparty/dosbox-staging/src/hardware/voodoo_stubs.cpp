// voodoo_stubs.cpp
// Stub implementations used when 3dfx Voodoo support is disabled/removed.

#include <cstddef>
#include <memory>

// Forward declarations to avoid pulling in many headers.
// (Match the signatures used by the callers.)
class Config;

extern "C" {

// memory.cpp expects this symbol
void *VOODOO_PCI_GetLFBPageHandler(unsigned long /*page*/)
{
    return nullptr;
}

} // extern "C"

// dosbox.cpp expects this symbol (C++)
void VOODOO_AddConfigSection(const std::unique_ptr<Config> & /*config*/)
{
    // no-op
}
