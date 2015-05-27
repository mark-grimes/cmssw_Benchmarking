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
			void enableMemoryCounter( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor );
			void disableMemoryCounterAndPrint( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor );

			void preModuleConstruction( const edm::ModuleDescription& description );
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
		activityRegister.watchPostModuleConstruction( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "Construction";} ) );

		activityRegister.watchPreModuleBeginJob( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "beginJob";} ) );
		activityRegister.watchPostModuleBeginJob( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "beginJob";} ) );

		activityRegister.watchPreModuleBeginRun( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "beginRun";} ) );
		activityRegister.watchPostModuleBeginRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "beginRun";} ) );

		activityRegister.watchPreModuleBeginLumi( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "beginLumi";} ) );
		activityRegister.watchPostModuleBeginLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "beginLumi";} ) );

		activityRegister.watchPreModule( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, [&]{return "event"+std::to_string(pImple_->eventNumber_);} ) );
		activityRegister.watchPostModule( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, [&]{return "event"+std::to_string(pImple_->eventNumber_);} ) );

		activityRegister.watchPostProcessEvent( [&](const edm::Event&,const edm::EventSetup&){++pImple_->eventNumber_;} );

		activityRegister.watchPreModuleEndLumi( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "endLumi";} ) );
		activityRegister.watchPostModuleEndLumi( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "endLumi";} ) );

		activityRegister.watchPreModuleEndRun( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "endRun";} ) );
		activityRegister.watchPostModuleEndRun( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "endRun";} ) );

		activityRegister.watchPreModuleEndJob( std::bind( &MemoryCounterPimple::enableMemoryCounter, pImple_, std::placeholders::_1, []{return "endJob";} ) );
		activityRegister.watchPostModuleEndJob( std::bind( &MemoryCounterPimple::disableMemoryCounterAndPrint, pImple_, std::placeholders::_1, []{return "endJob";} ) );
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

void markstools::services::MemoryCounterPimple::enableMemoryCounter( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor )
{
	const std::string& methodName=methodNameFunctor();

	auto iModuleDetails=memoryCounters_.find( description.moduleLabel() );
	if( iModuleDetails!=memoryCounters_.end() )
	{
		iModuleDetails->second.pMemoryCounter->resetMaximum();
		if( iModuleDetails->second.previousRecordedSize!=-1 ) iModuleDetails->second.previousRecordedSize-=iModuleDetails->second.pMemoryCounter->currentSize();
		iModuleDetails->second.pMemoryCounter->enable();

		if( verbose_ ) std::cout << "Enabling MemCounter for module \"" << description.moduleLabel() << "\" in method " << methodName << "." << std::endl;
	}
}

void markstools::services::MemoryCounterPimple::disableMemoryCounterAndPrint( const edm::ModuleDescription& description, std::function<std::string()> methodNameFunctor )
{
	const std::string& methodName=methodNameFunctor();

	auto iModuleDetails=memoryCounters_.find( description.moduleLabel() );
	if( iModuleDetails!=memoryCounters_.end() )
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
