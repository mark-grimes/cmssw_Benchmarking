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
		activityRegister.watchPreModuleConstruction( this, &MemoryCounter::preModuleConstruction );
		activityRegister.watchPostModuleConstruction( this, &MemoryCounter::postModuleConstruction );

		activityRegister.watchPreModuleBeginJob( this, &MemoryCounter::preModuleBeginJob );
		activityRegister.watchPostModuleBeginJob( this, &MemoryCounter::postModuleBeginJob );

		activityRegister.watchPreModuleBeginRun( this, &MemoryCounter::preModuleBeginRun );
		activityRegister.watchPostModuleBeginRun( this, &MemoryCounter::postModuleBeginRun );

		activityRegister.watchPreModuleBeginLumi( this, &MemoryCounter::preModuleBeginLumi );
		activityRegister.watchPostModuleBeginLumi( this, &MemoryCounter::postModuleBeginLumi );

		activityRegister.watchPreModule( this, &MemoryCounter::preModule );
		activityRegister.watchPostModule( this, &MemoryCounter::postModule );

		activityRegister.watchPostProcessEvent( this, &MemoryCounter::postProcessEvent );

		activityRegister.watchPreModuleEndLumi( this, &MemoryCounter::preModuleEndLumi );
		activityRegister.watchPostModuleEndLumi( this, &MemoryCounter::postModuleEndLumi );

		activityRegister.watchPreModuleEndRun( this, &MemoryCounter::preModuleEndRun );
		activityRegister.watchPostModuleEndRun( this, &MemoryCounter::postModuleEndRun );

		activityRegister.watchPreModuleEndJob( this, &MemoryCounter::preModuleEndJob );
		activityRegister.watchPostModuleEndJob( this, &MemoryCounter::postModuleEndJob );
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

markstools::services::MemoryCounter::MemoryCounter( const MemoryCounter& otherMemoryCounter )
	: pImple_(new MemoryCounterPimple(*(otherMemoryCounter.pImple_)) )
{
	// Since this is a service, the copy constructor should never be called
	// but I might as well put it in just in case.
	// No deep copy commands required at the moment.
}

markstools::services::MemoryCounter& markstools::services::MemoryCounter::operator=( const MemoryCounter& otherMemoryCounter )
{
	// Since this is a service, assignment should never be called
	// but I might as well put it in just in case.

	if( &otherMemoryCounter==this ) return *this; // check for self assignment
	*pImple_=*(otherMemoryCounter.pImple_); // copy the contents of the other pimple into this pimple

	// No deep copy commands required at the moment.
	return *this;
}

void markstools::services::MemoryCounter::preModuleConstruction( const edm::ModuleDescription& description )
{
	// If the vector is empty then I went to analyse all the modules. Otherwise check to see if the module name is in the list of modules to be analysed
	if( pImple_->modulesToAnalyse_.empty() || std::find( pImple_->modulesToAnalyse_.begin(), pImple_->modulesToAnalyse_.end(), description.moduleLabel() )!=pImple_->modulesToAnalyse_.end() )
	{
		memcounter::IMemoryCounter* pMemoryCounter=pImple_->createNewMemoryCounter();
		if( pMemoryCounter )
		{
			pImple_->memoryCounters_.insert( std::make_pair(description.moduleLabel(),::ModuleDetails(pMemoryCounter)) );
			if( pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" of type \"" << description.moduleName() << "\"." << std::endl;
			pMemoryCounter->resetMaximum();
			pMemoryCounter->enable();
		}
		else std::cout << "Couldn't get a pointer for a new IMemoryAnalyser, so module \"" << description.moduleLabel() << "\" will not be analysed." << std::endl;
	}
	else std::cout << "MemCounter not enabled for module \"" << description.moduleLabel() << "\"." << std::endl;
}

void markstools::services::MemoryCounter::postModuleConstruction( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "Construction" );
}

void markstools::services::MemoryCounter::preModuleBeginJob( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, pImple_->memoryCounters_ );
	if( isEnabled && pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method beginJob." << std::endl;
}

void markstools::services::MemoryCounter::postModuleBeginJob( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "beginJob" );
}

void markstools::services::MemoryCounter::preModuleBeginRun( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, pImple_->memoryCounters_ );
	if( isEnabled && pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method beginRun." << std::endl;
}

void markstools::services::MemoryCounter::postModuleBeginRun( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "beginRun" );
}

void markstools::services::MemoryCounter::preModuleBeginLumi( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, pImple_->memoryCounters_ );
	if( isEnabled && pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method beginLuminosityBlock." << std::endl;
}

void markstools::services::MemoryCounter::postModuleBeginLumi( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "beginLumi" );
}

void markstools::services::MemoryCounter::preModule( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, pImple_->memoryCounters_ );
	if( isEnabled && pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method analyze." << std::endl;
}

void markstools::services::MemoryCounter::postModule( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "event"+std::to_string(pImple_->eventNumber_) );
}

void markstools::services::MemoryCounter::postProcessEvent( const edm::Event& event, const edm::EventSetup& eventSetup )
{
	++pImple_->eventNumber_;
}

void markstools::services::MemoryCounter::preModuleEndLumi( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, pImple_->memoryCounters_ );
	if( isEnabled && pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method endLuminosityBlock." << std::endl;
}

void markstools::services::MemoryCounter::postModuleEndLumi( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "endLumi" );
}

void markstools::services::MemoryCounter::preModuleEndRun( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, pImple_->memoryCounters_ );
	if( isEnabled && pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method endRun." << std::endl;
}

void markstools::services::MemoryCounter::postModuleEndRun( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "endRun" );
}

void markstools::services::MemoryCounter::preModuleEndJob( const edm::ModuleDescription& description )
{
	bool isEnabled=::enableMemoryCounter( description, pImple_->memoryCounters_ );
	if( isEnabled && pImple_->verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method endJob." << std::endl;
}

void markstools::services::MemoryCounter::postModuleEndJob( const edm::ModuleDescription& description )
{
	::disableMemoryCounterAndPrint( description, pImple_->memoryCounters_, "endJob" );
}
