// Linux bring-up bridge for Archetype::GCliObjFactory.
// Provides a compatible ICliObjFactory/FCliObjFactory instance for init/data paths
// until the full client_cliobj TU and its runtime dependencies are linked.
// Archetype loaders are currently ABI-guarded on Linux in DataConfig, so Create()
// is not expected to run in the current bring-up flow; keep it safe.

#include "client_cliobj.h"

namespace Archetype
{
  void FCliObjFactory::Create(int /*TypeId*/, ICliObj** Out)
  {
    if (Out)
      *Out = nullptr;
  }

  FCliObjFactory GCliObjFactory;
}
