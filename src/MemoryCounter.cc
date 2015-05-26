#include "MarksTools/Benchmarking/interface/MemoryCounter.h"

#include <DataFormats/Provenance/interface/ModuleDescription.h>
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ServiceRegistry/interface/ActivityRegistry.h"
#include <dlfcn.h>

#include <iostream>
#include <fstream>

// This is the interface from the memory counter program. The include location is set in
// the BuildFile.xml.
#include <memcounter/IMemoryCounter.h>

//
// unnamed namespace for things only used in this file
//
namespace
{
	struct ModuleDetails
	{
		memcounter::IMemoryCounter* pMemoryCounter;
		long int previousRecordedSize;
		std::string previousEvent;
		ModuleDetails() : pMemoryCounter(nullptr), previousRecordedSize(-1) {}
		ModuleDetails( memcounter::IMemoryCounter* pNewCounter ) : pMemoryCounter(pNewCounter), previousRecordedSize(-1) {}
	};

	bool enableMemoryCounter( const edm::ModuleDescription& description, std::map<std::string,::ModuleDetails>& allModuleDetails )
	{
		auto iModuleDetails=allModuleDetails.find( description.moduleLabel() );
		if( iModuleDetails!=allModuleDetails.end() )
		{
			iModuleDetails->second.pMemoryCounter->resetMaximum();
			if( iModuleDetails->second.previousRecordedSize!=-1 ) iModuleDetails->second.previousRecordedSize-=iModuleDetails->second.pMemoryCounter->currentSize();
			iModuleDetails->second.pMemoryCounter->enable();
			return true;
		}
		return false;
	}

	void disableMemoryCounterAndPrint( const edm::ModuleDescription& description, std::map<std::string,::ModuleDetails>& allModuleDetails, const std::string& methodName )
	{
		auto iModuleDetails=allModuleDetails.find( description.moduleLabel() );
		if( iModuleDetails!=allModuleDetails.end() )
		{
			memcounter::IMemoryCounter* pMemoryCounter=iModuleDetails->second.pMemoryCounter;
			pMemoryCounter->disable();

			std::cout << " *MEMCOUNTER* " << methodName << "," << description.moduleLabel() << "," << description.moduleName()
					<< "," << pMemoryCounter->currentSize() << "," << pMemoryCounter->maximumSize()
					<< "," << pMemoryCounter->currentNumberOfAllocations() << "," << pMemoryCounter->maximumNumberOfAllocations();
			if( iModuleDetails->second.previousRecordedSize!=-1 ) std::cout << "," << iModuleDetails->second.previousEvent
					<< "," << iModuleDetails->second.previousRecordedSize;
			std::cout << std::endl;

			iModuleDetails->second.previousRecordedSize=pMemoryCounter->currentSize();
			iModuleDetails->second.previousEvent=methodName;
		}
	}
} // end of the unnamed namespace

//
// Define the pimple class
//
namespace markstools
{
	namespace services
	{
		class MemoryCounterPimple
		{
		public:
			MemoryCounterPimple() : eventNumber_(1), verbose_(false), createNewMemoryCounter(NULL) {}
			std::map<std::string,::ModuleDetails> memoryCounters_;
			size_t eventNumber_;
			std::vector<std::string> modulesToAnalyse_;
			bool verbose_;
			memcounter::IMemoryCounter* (*createNewMemoryCounter)( void );
			std::map<memcounter::IMemoryCounter*,long int> previousRecordedSize_; // The size recorded for the previous event. Used to calculate event content size.
		public:
			void preModuleConstruction( const edm::ModuleDescription& description );
			void postModuleConstruction( const edm::ModuleDescription& description );

			void preModuleBeginJob( const edm::ModuleDescription& description );
			void postModuleBeginJob( const edm::ModuleDescription& description );

			void preModuleBeginRun( const edm::ModuleDescription& description );
			void postModuleBeginRun( const edm::ModuleDescription& description );

			void preModuleBeginLumi( const edm::ModuleDescription& description );
			void postModuleBeginLumi( const edm::ModuleDescription& description );

			void preModule( const edm::ModuleDescription& description );
			void postModule( const edm::ModuleDescription& description );

			void postProcessEvent( const edm::Event& event, const edm::EventSetup& eventSetup );

			void preModuleEndLumi( const edm::ModuleDescription& description );
			void postModuleEndLumi( const edm::ModuleDescription& description );

			void preModuleEndRun( const edm::ModuleDescription& description );
			void postModuleEndRun( const edm::ModuleDescription& description );

			void preModuleEndJob( const edm::ModuleDescription& description );
			void postModuleEndJob( const edm::ModuleDescription& description );
		}; // end of the PlottingTimerPimple class

	} // end of the markstools::services namespace
} // end of the markstools namespace

markstools::services::MemoryCounter::MemoryCounter( const edm::ParameterSet& parameterSet, edm::ActivityRegistry& activityRegister )
	: pImple_(new MemoryCounterPimple)
{
	//
	// Try and get the interface to the memory counter from the preloaded library.
	//
	using memcounter::IMemoryCounter;
	if( void *sym = dlsym(0, "createNewMemoryCounter") )
	{
		pImple_->createNewMemoryCounter = __extension__(IMemoryCounter*(*)(void)) sym;

		//
		// Get some preferences from the config file and decide which modules to analyse
		//
		if( parameterSet.exists("modulesToAnalyse") ) pImple_->modulesToAnalyse_=parameterSet.getParameter< std::vector<std::string> >("modulesToAnalyse");
		else std::cout << "MemoryCounter: the parameter \"modulesToAnalyse\" has not been set, so MemoryCounter will analyse all modules" << std::endl;

		if( parameterSet.exists("verbose") ) pImple_->verbose_=parameterSet.getParameter<bool>("verbose");

		//
		// Register all of the watching functions
		//
		activityRegister.watchPreModuleConstruction( pImple_, &MemoryCounterPimple::preModuleConstruction );
		activityRegister.watchPostModuleConstruction( pImple_, &MemoryCounterPimple::postModuleConstruction );

		activityRegister.watchPreModuleBeginJob( pImple_, &MemoryCounterPimple::preModuleBeginJob );
		activityRegister.watchPostModuleBeginJob( pImple_, &MemoryCounterPimple::postModuleBeginJob );

		activityRegister.watchPreModuleBeginRun( pImple_, &MemoryCounterPimple::preModuleBeginRun );
		activityRegister.watchPostModuleBeginRun( pImple_, &MemoryCounterPimple::postModuleBeginRun );

		activityRegister.watchPreModuleBeginLumi( pImple_, &MemoryCounterPimple::preModuleBeginLumi );
		activityRegister.watchPostModuleBeginLumi( pImple_, &MemoryCounterPimple::postModuleBeginLumi );

		activityRegister.watchPreModule( pImple_, &MemoryCounterPimple::preModule );
		activityRegister.watchPostModule( pImple_, &MemoryCounterPimple::postModule );

		activityRegister.watchPostProcessEvent( pImple_, &MemoryCounterPimple::postProcessEvent );

		activityRegister.watchPreModuleEndLumi( pImple_, &MemoryCounterPimple::preModuleEndLumi );
		activityRegister.watchPostModuleEndLumi( pImple_, &MemoryCounterPimple::postModuleEndLumi );

		activityRegister.watchPreModuleEndRun( pImple_, &MemoryCounterPimple::preModuleEndRun );
		activityRegister.watchPostModuleEndRun( pImple_, &MemoryCounterPimple::postModuleEndRun );

		activityRegister.watchPreModuleEndJob( pImple_, &MemoryCounterPimple::preModuleEndJob );
		activityRegister.watchPostModuleEndJob( pImple_, &MemoryCounterPimple::postModuleEndJob );
	}
	else
	{
		std::cerr << " ***" << "\n"
				<< " *** MemoryCounter: couldn't get the symbol in the analysing library. Are you running under intrusiveMemoryAnalyser? MemoryCounter will not function without it." << "\n"
				<< " ***                See https://github.com/mark-grimes/MemCounter for details on how to get intrusiveMemoryAnalyser installed." << "\n"
				<< " ***" << std::endl;
	}
}

markstools::services::MemoryCounter::~MemoryCounter()
{
	delete pImple_;
}

void markstools::services::MemoryCounterPimple::preModuleConstruction( const edm::ModuleDescription& description )
{
	// If the vector is empty then I went to analyse all the modules. Otherwise check to see if the module name is in the list of modules to be analysed
	if( modulesToAnalyse_.empty() || std::find( modulesToAnalyse_.begin(), modulesToAnalyse_.end(), description.moduleLabel() )!=modulesToAnalyse_.end() )
	{
		memcounter::IMemoryCounter* pMemoryCounter=createNewMemoryCounter();
		if( pMemoryCounter )
		{
			memoryCounters_.insert( std::make_pair(description.moduleLabel(),::ModuleDetails(pMemoryCounter)) );
			if( verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" of type \"" << description.moduleName() << "\"." << std::endl;
			pMemoryCounter->resetMaximum();
			pMemoryCounter->enable();
		}
		else std::cout << "Couldn't get a pointer for a new IMemoryAnalyser, so module \"" << description.moduleLabel() << "\" will not be analysed." << std::endl;
	}
	else std::cout << "MemCounter not enabled for module \"" << description.moduleLabel() << "\"." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModuleConstruction( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "Construction" );
}

void markstools::services::MemoryCounterPimple::preModuleBeginJob( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, memoryCounters_ );
	if( isEnabled && verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method beginJob." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModuleBeginJob( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "beginJob" );
}

void markstools::services::MemoryCounterPimple::preModuleBeginRun( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, memoryCounters_ );
	if( isEnabled && verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method beginRun." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModuleBeginRun( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "beginRun" );
}

void markstools::services::MemoryCounterPimple::preModuleBeginLumi( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, memoryCounters_ );
	if( isEnabled && verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method beginLuminosityBlock." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModuleBeginLumi( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "beginLumi" );
}

void markstools::services::MemoryCounterPimple::preModule( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, memoryCounters_ );
	if( isEnabled && verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method analyze." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModule( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "event"+std::to_string(eventNumber_) );
}

void markstools::services::MemoryCounterPimple::postProcessEvent( const edm::Event& event, const edm::EventSetup& eventSetup )
{
	++eventNumber_;
}

void markstools::services::MemoryCounterPimple::preModuleEndLumi( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, memoryCounters_ );
	if( isEnabled && verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method endLuminosityBlock." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModuleEndLumi( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "endLumi" );
}

void markstools::services::MemoryCounterPimple::preModuleEndRun( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, memoryCounters_ );
	if( isEnabled && verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method endRun." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModuleEndRun( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "endRun" );
}

void markstools::services::MemoryCounterPimple::preModuleEndJob( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, memoryCounters_ );
	if( isEnabled && verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method endJob." << std::endl;
}

void markstools::services::MemoryCounterPimple::postModuleEndJob( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, memoryCounters_, "endJob" );
}
