#include "TrackSubsetAlg.h"

#include "GearSvc/IGearSvc.h"
#include "TrackSystemSvc/ITrackSystemSvc.h"
#include "DataHelper/Navigation.h"

#include <UTIL/ILDConf.h>

#include <gear/BField.h>

#include "KiTrack/SubsetSimple.h"
#include "KiTrack/SubsetHopfieldNN.h"
#include "Tools/Fitter.h"
#include "Tools/KiTrackMarlinTools.h"

#include "TrackSystemSvc/MarlinTrkUtils.h"

#include <TStopwatch.h>

using namespace KiTrack;

DECLARE_COMPONENT(TrackSubsetAlg)

TrackSubsetAlg::TrackSubsetAlg(const std::string& name, ISvcLocator* svcLoc)
  : GaudiAlgorithm(name, svcLoc){
  
  // modify processor description
  //_description = "TrackSubsetAlg takes tracks from multiple sources and outputs them (or modified versions, or a subset of them) as one track collection." ;
  
  //std::vector< std::string > trackInputColNamesDefault;
  //trackInputColNamesDefault.push_back( "ForwardTracks" );
  //trackInputColNamesDefault.push_back( "SiTracks" );
  
  declareProperty("TrackSubsetCollection", _outColHdl, "Handle of the SiTrack output collection");
}

StatusCode TrackSubsetAlg::initialize() { 

  debug() << "   init called  " << endmsg;

  _nRun = 0 ;
  _nEvt = 0 ;

  // Set up the track fit tool
  m_fitTool = ToolHandle<ITrackFitterTool>(m_fitToolName.value());

  if(m_dumpTime){
    NTuplePtr nt1(ntupleSvc(), "MyTuples/Time"+name());
    if ( !nt1 ) {
      m_tuple = ntupleSvc()->book("MyTuples/Time"+name(),CLID_ColumnWiseTuple,"Tracking time");
      if ( 0 != m_tuple ) {
	m_tuple->addItem ("timeTotal", m_timeTotal ).ignore();
	m_tuple->addItem ("timeKalman", m_timeKalman ).ignore();
      }
      else {
	fatal() << "Cannot book MyTuples/Time"+name() <<endmsg;
	return StatusCode::FAILURE;
      }
    }
    else{
      m_tuple = nt1;
    }
  }

  for(unsigned i=0; i<_trackInputColNames.size(); i++){
    _inTrackColHdls.push_back(new DataHandle<edm4hep::TrackCollection> (_trackInputColNames[i], Gaudi::DataHandle::Reader, this));
  }

  for(unsigned i=0; i<_trackerHitInputColNames.size(); i++){
    _inTrackerHitColHdls.push_back(new DataHandle<edm4hep::TrackerHit3DCollection> (_trackerHitInputColNames[i], Gaudi::DataHandle::Reader, this));
  }
  /**********************************************************************************************/
  /*       Initialise the MarlinTrkSystem, needed by the tracks for fitting                     */
  /**********************************************************************************************/
  auto _gear = service<IGearSvc>("GearSvc");
  if ( !_gear ) {
    error() << "Failed to find GearSvc ..." << endmsg;
    return StatusCode::FAILURE;
  }
  gear::GearMgr* gearMgr = _gear->getGearMgr();
  _bField = gearMgr->getBField().at( gear::Vector3D( 0.,0.,0.)  ).z() ;
    
  // set upt the geometry
  auto _trackSystemSvc = service<ITrackSystemSvc>("TrackSystemSvc");
  if ( !_trackSystemSvc ) {
    error() << "Failed to find TrackSystemSvc ..." << endmsg;
    return StatusCode::FAILURE;
  }
  _trkSystem =  _trackSystemSvc->getTrackSystem(this);

  if( _trkSystem == 0 ){
    error() << "Cannot initialize MarlinTrkSystem of Type: KalTest" <<endmsg;
    return StatusCode::FAILURE;
  }
  
  // set the options   
  _trkSystem->setOption( MarlinTrk::IMarlinTrkSystem::CFG::useQMS,        _MSOn ) ;       //multiple scattering
  _trkSystem->setOption( MarlinTrk::IMarlinTrkSystem::CFG::usedEdx,       _ElossOn) ;     //energy loss
  _trkSystem->setOption( MarlinTrk::IMarlinTrkSystem::CFG::useSmoothing,  _SmoothOn) ;    //smoothing
  
  // initialise the tracking system
  _trkSystem->init() ;

  return GaudiAlgorithm::initialize();
}

StatusCode TrackSubsetAlg::finalize(){
  for(unsigned i=0; i<_inTrackColHdls.size(); i++){
    delete _inTrackColHdls[i];
  }
  _inTrackColHdls.clear();

  for(unsigned i=0; i<_inTrackerHitColHdls.size(); i++){
    delete _inTrackerHitColHdls[i];
  }
  _inTrackerHitColHdls.clear();

  return GaudiAlgorithm::finalize();
}

StatusCode TrackSubsetAlg::execute(){ 
  auto stopwatch = TStopwatch();

  std::vector<edm4hep::Track> tracks;

  auto trkCol = _outColHdl.createAndPut();
  /**********************************************************************************************/
  /*       Read in the collections                                                              */
  /**********************************************************************************************/
  debug() << "Try to load " << _trackInputColNames.size() << " input track collections" << endmsg;
  
  unsigned nTrackLoaded = 0;
  
  for( unsigned i=0; i < _inTrackColHdls.size(); i++ ){
    const edm4hep::TrackCollection* trackCol = nullptr;
    try {
      trackCol = _inTrackColHdls[i]->get();
    }
    catch ( GaudiException &e ) {
      debug() << "Collection " << _inTrackColHdls[i]->fullKey() << " is unavailable in event " << _nEvt << endmsg;
      continue;
    }

    if(trackCol){
      int nTracks = trackCol->size();
      debug() << "Load track input collection " << _trackInputColNames[i] << " with " << nTracks << " tracks" << endmsg;
            
      for(auto track : *trackCol){
	tracks.push_back( track );
	nTrackLoaded++;
      }        
    }
    else error() << "track input collection " << _trackInputColNames[i] << " could not be found, but not throw exception!!!" << endmsg;
  }

  debug() << "Loaded all in all " << nTrackLoaded << " tracks, which will now get further processed" << endmsg;
  if(tracks.size()==0) return StatusCode::SUCCESS;

  Navigation::Instance()->Initialize();
  for(unsigned i=0; i < _inTrackerHitColHdls.size(); i++){
    try {
      auto trackCol = _inTrackerHitColHdls[i]->get();
      Navigation::Instance()->AddTrackerHitCollection(trackCol);
    }
    catch ( GaudiException &e ) {
      debug() << "Collection " << _inTrackerHitColHdls[i]->fullKey() << " is unavailable in event " << _nEvt << endmsg;
      continue;
    }
  }
  /**********************************************************************************************/
  /*       Make sure that all tracks are compatible: find the best subset                       */
  /**********************************************************************************************/
  debug() << "Find the best subset of tracks using the Hopfield Neural Network" << endmsg;
  
  TrackQI trackQI( _trkSystem );
  
  debug() << "The tracks and their qualities (and their hits ): " << endmsg;

  std::vector<edm4hep::Track*> tracks_p;
  for( unsigned i=0; i < tracks.size(); i++ ){
    auto* track = &tracks[i];
    tracks_p.push_back(track);
    double qi = trackQI( track );
    debug() << "Track " << track->id() << " address " << track << "\t" << qi << "( ";
    std::vector<edm4hep::TrackerHit3D> hits;
    hits.push_back(edm4hep::TrackerHit3D(hit.getCellID(),
                                         hit.getType(),
                                         hit.getQuality(),
                                         hit.getTime(),
                                         hit.getEDep(),
                                         hit.getEDepError(),
                                         hit.getPosition(),
                                         {}));
    
    std::sort( hits.begin(), hits.end(), KiTrackMarlin::compare_TrackerHit_z );
    
    for( unsigned j=0; j<hits.size(); j++ ){
      debug() << hits[j].id() << " ";
      double x = hits[j].getPosition()[0];
      double y = hits[j].getPosition()[1];
      double z = hits[j].getPosition()[2];
      debug() << "[" << x << "," << y << "," << z << "]";
    }
    debug() << ")" << endmsg;
  }
  
  TrackCompatibility comp;
  
  SubsetHopfieldNN<edm4hep::Track*> subset;
  //SubsetSimple<edm4hep::Track* > subset;
  subset.add( tracks_p );
  subset.setOmega( _omega );
  subset.calculateBestSet( comp, trackQI );

  auto accepted = subset.getAccepted();
  auto rejected = subset.getRejected();
  
  debug() << "\tThe accepted tracks:" << endmsg;
  for( unsigned i=0; i < accepted.size(); i++ ){
    debug() << accepted[i]->id() << " address " << accepted[i] << endmsg;
  }
  
  debug() << "\tThe rejected tracks:" << endmsg;
  for( unsigned i=0; i < rejected.size(); i++ ){
    debug() << rejected[i]->id() << " address " << rejected[i] << endmsg;
  }
  
  /**********************************************************************************************/
  /*            Save the tracks to a collection (make new TrackImpls from them)                 */
  /**********************************************************************************************/
  debug() << "Fitting and saving of the tracks" << endmsg;

  //auto trkCol = _outColHdl.createAndPut();
  auto stopwatch_kalman = TStopwatch();
  for( unsigned i=0; i < accepted.size(); i++ ){
    edm4hep::MutableTrack trackImpl;
    
    auto track = accepted[i];
    
    std::vector<edm4hep::TrackerHit3D> trackerHitsObj;
    std::vector<edm4hep::TrackerHit3D> trackerHits;
    std::copy(track->trackerHits_begin(), track->trackerHits_end(), std::back_inserter(trackerHitsObj));

    for(unsigned i=0; i<trackerHitsObj.size(); i++){
      //debug() << trackerHitsObj[i].id() << endmsg;
      trackerHits.push_back(Navigation::Instance()->GetTrackerHit(trackerHitsObj[i].getObjectID()));
    } 
    // setup initial dummy covariance matrix
    decltype(edm4hep::TrackState::covMatrix) covMatrix;
    for (unsigned icov = 0; icov<covMatrix.size(); ++icov) {
      covMatrix[icov] = 0;
    }
    
    covMatrix[0]  = ( _initialTrackError_d0    ); //sigma_d0^2
    covMatrix[2]  = ( _initialTrackError_phi0  ); //sigma_phi0^2
    covMatrix[5]  = ( _initialTrackError_omega ); //sigma_omega^2
    covMatrix[9]  = ( _initialTrackError_z0    ); //sigma_z0^2
    covMatrix[14] = ( _initialTrackError_tanL  ); //sigma_tanl^2
    
    std::vector< std::pair<float, edm4hep::TrackerHit3D> > r2_values;
    r2_values.reserve(trackerHits.size());
    
    for (std::vector<edm4hep::TrackerHit3D>::iterator it=trackerHits.begin(); it!=trackerHits.end(); ++it) {
      edm4hep::TrackerHit3D h = *it;
      float r2 = h.getPosition()[0]*h.getPosition()[0]+h.getPosition()[1]*h.getPosition()[1];
      r2_values.push_back(std::make_pair(r2, *it));
    }
    
    sort(r2_values.begin(),r2_values.end());
    
    trackerHits.clear();
    trackerHits.reserve(r2_values.size());
    
    for (std::vector< std::pair<float, edm4hep::TrackerHit3D> >::iterator it=r2_values.begin(); it!=r2_values.end(); ++it) {
      trackerHits.push_back(it->second);
    }

    bool fit_backwards = MarlinTrk::IMarlinTrack::backward;
    
    //MarlinTrk::IMarlinTrack* marlinTrk = _trkSystem->createTrack();
    
    int error = 0;
    
    try {
      
      //error = MarlinTrk::createFinalisedLCIOTrack(marlinTrk, trackerHits, &trackImpl, fit_backwards, covMatrix, _bField, _maxChi2PerHit);
      error = m_fitTool->Fit(trackImpl, trackerHits, covMatrix, _maxChi2PerHit, fit_backwards);

    } catch (...) {
      
      //      delete Track;
      //      delete marlinTrk;
      
      throw ;
      
    }
    
    // Add hit numbers 
    
    std::vector<std::pair<edm4hep::TrackerHit3D , double> > hits_in_fit ;
    std::vector<std::pair<edm4hep::TrackerHit3D , double> > outliers ;
    std::vector<edm4hep::TrackerHit3D> all_hits;
    //all_hits.reserve(300);
    
    //marlinTrk->getHitsInFit(hits_in_fit);
    hits_in_fit = m_fitTool->GetHitsInFit();

    for ( unsigned ihit = 0; ihit < hits_in_fit.size(); ++ihit) {
      all_hits.push_back(hits_in_fit[ihit].first);
    }
    
    UTIL::BitField64 cellID_encoder( lcio::ILDCellID0::encoder_string ) ;
    
    MarlinTrk::addHitNumbersToTrack(&trackImpl, all_hits, true, cellID_encoder);
    
    //marlinTrk->getOutliers(outliers);
    outliers = m_fitTool->GetOutliers();

    for ( unsigned ihit = 0; ihit < outliers.size(); ++ihit) {
      all_hits.push_back(outliers[ihit].first);
    }
    
    MarlinTrk::addHitNumbersToTrack(&trackImpl, all_hits, false, cellID_encoder);
    
    //delete marlinTrk;
    m_fitTool->Clear();
        
    if( error != MarlinTrk::IMarlinTrack::success ) {
      //delete trackImpl;
      debug() << "TrackSubsetAlg:: Track fit failed with error code " << error << " track dropped. Number of hits = "<< trackerHits.size() << endmsg;
      continue ;
    }
    
    if( trackImpl.getNdf() < 0) {
      //delete trackImpl;
      debug() << "TrackSubsetAlg:: Track fit returns " << trackImpl.getNdf() << " degress of freedom track dropped. Number of hits = "<< trackerHits.size() << endmsg;
      continue ;
    }
    
    trkCol->push_back(trackImpl);
    
//    try{
//
//      Fitter fitter( trackImpl , _trkSystem );
//
//      TrackStateImpl* trkStateIP = new TrackStateImpl( fitter.getTrackState( lcio::TrackState::AtIP ) ) ;
//      trackImpl->setChi2( fitter.getChi2( lcio::TrackState::AtIP ) );
//      trackImpl->setNdf( fitter.getNdf( lcio::TrackState::AtIP ) );
//      trkStateIP->setLocation( TrackState::AtIP );
//      trackImpl->addTrackState( trkStateIP );
//      
//      trackVec->addElement(trackImpl);
//      
//    }
//    catch( FitterException e ){
//      
//      delete trackImpl;
//      
//      streamlog_out( ERROR ) << "TrackImpl nr " << i  << " rejected, because fit failed: " <<  e.what() << "\n";
//      continue;
//      
//    }
  }

  debug() << "Saving " << trkCol->size() << " tracks" << endmsg;
  
  Navigation::Instance()->Initialize();

  _nEvt ++ ;

  if(m_dumpTime&&m_tuple){
    m_timeTotal = stopwatch.RealTime()*1000;
    m_timeKalman = stopwatch_kalman.RealTime()*1000;
    m_tuple->write();
  }

  return StatusCode::SUCCESS;
}






