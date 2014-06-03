#include "ModuleTimer.h"
#include "FWCore/ServiceRegistry/interface/ServiceMaker.h" // Required for DEFINE_FWK_SERVICE

#include <DataFormats/Provenance/interface/ModuleDescription.h>

#include <iostream>
#include <boost/chrono/process_cpu_clocks.hpp>


//
// Define the pimple class
//
namespace markstools
{
	namespace services
	{
		class ModuleTimerPimple
		{
		public:
			ModuleTimerPimple() : eventNumber_(1), verbose_(false) {}
			size_t eventNumber_;
			bool verbose_;
			boost::chrono::process_cpu_clock::time_point moduleStartTime_;
			boost::chrono::process_cpu_clock::time_point eventStartTime_;
		}; // end of the PlottingTimerPimple class

	} // end of the markstools::services namespace
} // end of the markstools namespace

markstools::services::ModuleTimer::ModuleTimer( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister )
	: pImple_(new ModuleTimerPimple)
{
	//
	// Register all of the watching functions
	//
	activityRegister.watchPreModuleConstruction( this, &ModuleTimer::preModuleConstruction );
	activityRegister.watchPostModuleConstruction( this, &ModuleTimer::postModuleConstruction );

	activityRegister.watchPreModuleBeginJob( this, &ModuleTimer::preModuleBeginJob );
	activityRegister.watchPostModuleBeginJob( this, &ModuleTimer::postModuleBeginJob );

	activityRegister.watchPreModuleBeginRun( this, &ModuleTimer::preModuleBeginRun );
	activityRegister.watchPostModuleBeginRun( this, &ModuleTimer::postModuleBeginRun );

	activityRegister.watchPreModuleBeginLumi( this, &ModuleTimer::preModuleBeginLumi );
	activityRegister.watchPostModuleBeginLumi( this, &ModuleTimer::postModuleBeginLumi );

	activityRegister.watchPreModule( this, &ModuleTimer::preModule );
	activityRegister.watchPostModule( this, &ModuleTimer::postModule );

	activityRegister.watchPreProcessEvent( this, &ModuleTimer::preProcessEvent );
	activityRegister.watchPostProcessEvent( this, &ModuleTimer::postProcessEvent );

	activityRegister.watchPreModuleEndLumi( this, &ModuleTimer::preModuleEndLumi );
	activityRegister.watchPostModuleEndLumi( this, &ModuleTimer::postModuleEndLumi );

	activityRegister.watchPreModuleEndRun( this, &ModuleTimer::preModuleEndRun );
	activityRegister.watchPostModuleEndRun( this, &ModuleTimer::postModuleEndRun );

	activityRegister.watchPreModuleEndJob( this, &ModuleTimer::preModuleEndJob );
	activityRegister.watchPostModuleEndJob( this, &ModuleTimer::postModuleEndJob );
}

markstools::services::ModuleTimer::~ModuleTimer()
{
	delete pImple_;
}

void markstools::services::ModuleTimer::preModuleConstruction( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModuleConstruction( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* Construction," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

void markstools::services::ModuleTimer::preModuleBeginJob( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModuleBeginJob( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* beginJob," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

void markstools::services::ModuleTimer::preModuleBeginRun( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModuleBeginRun( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* beginRun," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

void markstools::services::ModuleTimer::preModuleBeginLumi( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModuleBeginLumi( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* beginLumi," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

void markstools::services::ModuleTimer::preModule( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModule( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* event" << pImple_->eventNumber_ << "," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

void markstools::services::ModuleTimer::preProcessEvent( const edm::EventID& eventID, const edm::Timestamp& timeStamp )
{
	pImple_->eventStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postProcessEvent( const edm::Event& event, const edm::EventSetup& eventSetup )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->eventStartTime_ );
	std::cout << " *MODULETIMER* event" << pImple_->eventNumber_ << ",EVENT,EVENT"
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
	++pImple_->eventNumber_;
}

void markstools::services::ModuleTimer::preModuleEndLumi( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModuleEndLumi( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* endLumi," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

void markstools::services::ModuleTimer::preModuleEndRun( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModuleEndRun( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* endRun," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

void markstools::services::ModuleTimer::preModuleEndJob( const edm::ModuleDescription& description )
{
	pImple_->moduleStartTime_=boost::chrono::process_cpu_clock::now();
}

void markstools::services::ModuleTimer::postModuleEndJob( const edm::ModuleDescription& description )
{
	boost::chrono::process_cpu_clock::duration timeTaken( boost::chrono::process_cpu_clock::now()-pImple_->moduleStartTime_ );
	std::cout << " *MODULETIMER* endJob," << description.moduleLabel() << "," << description.moduleName()
			<< "," << timeTaken.count().real << "," << timeTaken.count().user << "," << timeTaken.count().system << std::endl;
}

using markstools::services::ModuleTimer;
DEFINE_FWK_SERVICE( ModuleTimer );
