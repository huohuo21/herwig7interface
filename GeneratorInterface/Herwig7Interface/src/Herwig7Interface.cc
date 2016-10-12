/** \class Herwig7Interface
 *  
 *  Marco A. Harrendorf marco.harrendorf@cern.ch
 */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <cmath>
#include <stdlib.h>

#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

#include <HepMC/GenEvent.h>
#include <HepMC/PdfInfo.h>
#include <HepMC/IO_GenEvent.h>

#include <ThePEG/Utilities/DynamicLoader.h>
#include <ThePEG/Repository/Repository.h>
#include <ThePEG/Handlers/EventHandler.h>
#include <ThePEG/Handlers/XComb.h>
#include <ThePEG/EventRecord/Event.h>
#include <ThePEG/EventRecord/Particle.h> 
#include <ThePEG/EventRecord/Collision.h>
#include <ThePEG/EventRecord/TmpTransform.h>
#include <ThePEG/Config/ThePEG.h>
#include <ThePEG/PDF/PartonExtractor.h>
#include <ThePEG/PDF/PDFBase.h>
#include <ThePEG/Utilities/UtilityBase.h>
#include <ThePEG/Vectors/HepMCConverter.h>


#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "GeneratorInterface/Core/interface/ParameterCollector.h"

#include "GeneratorInterface/Herwig7Interface/interface/Proxy.h"
#include "GeneratorInterface/Herwig7Interface/interface/RandomEngineGlue.h"
#include "GeneratorInterface/Herwig7Interface/interface/Herwig7Interface.h"

using namespace std;
using namespace gen;

Herwig7Interface::Herwig7Interface(const edm::ParameterSet &pset) :
	randomEngineGlueProxy_(ThePEG::RandomEngineGlue::Proxy::create()),
	dataLocation_(ParameterCollector::resolve(pset.getParameter<string>("dataLocation"))),
	generator_(pset.getParameter<string>("generatorModule")),
	run_(pset.getParameter<string>("run")),
	dumpConfig_(pset.getUntrackedParameter<string>("dumpConfig", "")),
	skipEvents_(pset.getUntrackedParameter<unsigned int>("skipEvents", 0))
{
	// Write events in hepmc ascii format for debugging purposes
	string dumpEvents = pset.getUntrackedParameter<string>("dumpEvents", "");
	if (!dumpEvents.empty()) {
		iobc_.reset(new HepMC::IO_GenEvent(dumpEvents.c_str(), ios::out));
		edm::LogInfo("ThePEGSource") << "Event logging switched on (=> " << dumpEvents << ")";
	}
	// Clear dumpConfig target
	if (!dumpConfig_.empty())
		ofstream cfgDump(dumpConfig_.c_str(), ios_base::trunc);
}

Herwig7Interface::~Herwig7Interface()
{
	if (eg_)
		eg_->finalize();
	edm::LogInfo("Herwig7Interface") << "Event generator finalized";
}

void Herwig7Interface::setPEGRandomEngine(CLHEP::HepRandomEngine* v) {
        randomEngineGlueProxy_->setRandomEngine(v);
        ThePEG::RandomEngineGlue *rnd = randomEngineGlueProxy_->getInstance();
        if(rnd) {
          rnd->setRandomEngine(v);
        }
}

string Herwig7Interface::dataFile(const string &fileName) const
{
	if (fileName.empty() || fileName[0] == '/')
		return fileName;
	else
		return dataLocation_ + "/" + fileName;
}

string Herwig7Interface::dataFile(const edm::ParameterSet &pset,
	                         const string &paramName) const
{
	const edm::Entry &entry = pset.retrieve(paramName);
	if (entry.typeCode() == 'F')
		return entry.getFileInPath().fullPath();
	else
		return dataFile(entry.getString());
}

void Herwig7Interface::initRepository(const edm::ParameterSet &pset) const
{
	/* Initialize the repository from
	 * 1. the repository file (default values)
	 * 2. the ThePEG config files
	 * 3. the CMSSW config blocks
	 */
	stringstream logstream;

	// Read the repository of serialized default values
	string repository = dataFile(pset, "repository");
	if (!repository.empty()) {
		edm::LogInfo("Herwig7Interface") << "Loading repository (" << repository << ")";
		ThePEG::Repository::load(repository);
	}

	if (!getenv("ThePEG_INSTALL_PATH")) {
		vector<string> libdirlist = ThePEG::DynamicLoader::allPaths();
		for(vector<string>::const_iterator libdir = libdirlist.begin();
		    libdir < libdirlist.end(); ++libdir) {
			if (libdir->empty() || (*libdir)[0] != '/')
				continue;
			if (boost::filesystem::exists(*libdir +
					"/../../share/ThePEG/PDFsets.index")) {
				setenv("ThePEG_INSTALL_PATH",
				       libdir->c_str(), 0);
				break;
			}
		}
	}

	// Read ThePEG config files to read
	vector<string> configFiles = pset.getParameter<vector<string> >("configFiles");

	// Loop over the config files
	for(vector<string>::const_iterator iter = configFiles.begin();
	    iter != configFiles.end(); ++iter) {
		edm::LogInfo("Herwig7Interface") << "Reading config file (" << *iter << ")";
                ThePEG::Repository::read(dataFile(*iter), logstream);
                edm::LogInfo("ThePEGSource") << logstream.str();
	}

	// Read CMSSW config file parameter sets starting from "parameterSets"

	ofstream cfgDump;
	ParameterCollector collector(pset);
	ParameterCollector::const_iterator iter;
	if (!dumpConfig_.empty()) {
		cfgDump.open(dumpConfig_.c_str(), ios_base::app);
		iter = collector.begin(cfgDump);
	} else
		iter = collector.begin();

	for(; iter != collector.end(); ++iter) {
		string out = ThePEG::Repository::exec(*iter, logstream);
		if (!out.empty()) {
			edm::LogInfo("Herwig7Interface") << *iter << " => " << out;
			cerr << "Error in ThePEG configuration!\n"
			        "\tLine: " << *iter << "\n" << out << endl;
		}
	}

	if (!dumpConfig_.empty()) {
		cfgDump << "saverun " << run_ << " " << generator_ << endl;
		cfgDump.close();
	}

	// write the ProxyID for the RandomEngineGlue to fill its pointer in
	ostringstream ss;
	ss << randomEngineGlueProxy_->getID();
	ThePEG::Repository::exec("set " + generator_ +
	                         ":RandomNumberGenerator:ProxyID " + ss.str(),
	                         logstream);

	// Print the directories where ThePEG looks for libs
	vector<string> libdirlist = ThePEG::DynamicLoader::allPaths();
	for(vector<string>::const_iterator libdir = libdirlist.begin();
	    libdir < libdirlist.end(); ++libdir)
		edm::LogInfo("Herwig7Interface") << "DynamicLoader path = " << *libdir << endl;

	// Output status information about the repository
	ThePEG::Repository::stats(logstream);
	edm::LogInfo("Herwig7Interface") << logstream.str();
}

void Herwig7Interface::initGenerator()
{
	// Get generator from the repository and initialize it
	ThePEG::BaseRepository::CheckObjectDirectory(generator_);
	ThePEG::EGPtr tmp = ThePEG::BaseRepository::GetObject<ThePEG::EGPtr>(generator_);
	if (tmp) {
		eg_ = ThePEG::Repository::makeRun(tmp, run_);
		eg_->initialize();
		edm::LogInfo("Herwig7Interface") << "EventGenerator initialized";
	} else
		throw cms::Exception("Herwig7Interface")
			<< "EventGenerator could not be initialized!" << endl;

	// Skip events
	for (unsigned int i = 0; i < skipEvents_; i++) {
		flushRandomNumberGenerator();
		eg_->shoot();
		edm::LogInfo("Herwig7Interface") << "Event discarded";
	}
}

void Herwig7Interface::flushRandomNumberGenerator()
{
	ThePEG::RandomEngineGlue *rnd = randomEngineGlueProxy_->getInstance();

	if (!rnd)
		edm::LogWarning("ProxyMissing")
			<< "ThePEG not initialised with RandomEngineGlue.";
	else
		rnd->flush();
}

auto_ptr<HepMC::GenEvent> Herwig7Interface::convert(
					const ThePEG::EventPtr &event)
{
	return std::auto_ptr<HepMC::GenEvent>(
		ThePEG::HepMCConverter<HepMC::GenEvent>::convert(*event));
}




double Herwig7Interface::pthat(const ThePEG::EventPtr &event)
{
	using namespace ThePEG;

	if (!event->primaryCollision())
		return -1.0;

	tSubProPtr sub = event->primaryCollision()->primarySubProcess();
	TmpTransform<tSubProPtr> tmp(sub, Utilities::getBoostToCM(
							sub->incoming()));

	double pthat = (*sub->outgoing().begin())->momentum().perp();
	for(PVector::const_iterator it = sub->outgoing().begin();
	    it != sub->outgoing().end(); ++it)
		pthat = std::min(pthat, (*it)->momentum().perp());

	return pthat / ThePEG::GeV;
}
