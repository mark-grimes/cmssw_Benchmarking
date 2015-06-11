#include "MarksTools/Benchmarking/interface/MemoryCounter.h"
#include "MarksTools/Benchmarking/interface/IgprofDump.h"
#include "ModuleTimer.h"
#include "FWCore/ServiceRegistry/interface/ServiceMaker.h" // Required for DEFINE_FWK_SERVICE

using markstools::services::MemoryCounter;
DEFINE_FWK_SERVICE( MemoryCounter );

using markstools::services::IgprofDump;
DEFINE_FWK_SERVICE( IgprofDump );

using markstools::services::ModuleTimer;
DEFINE_FWK_SERVICE( ModuleTimer );
