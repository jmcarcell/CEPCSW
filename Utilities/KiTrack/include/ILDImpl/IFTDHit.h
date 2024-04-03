#ifndef IFTDHit_h
#define IFTDHit_h


#if __has_include("edm4hep/TrackerHit3D.h")
#include "edm4hep/TrackerHit3D.h"
#else
#include "edm4hep/TrackerHit.h"
namespace edm4hep {
  using TrackerHit3D = edm4hep::TrackerHit;
} // namespace edm4hep
#endif

#include "KiTrack/IHit.h"

#include "ILDImpl/SectorSystemFTD.h"

namespace KiTrackMarlin{
  /** An interface for a hit for the ILD using an lcio TrackerHit as basis.
   * 
   * It comes along with a side, layer, module and a sensor.
   */   
  class IFTDHit : public IHit{
  public:
        
    edm4hep::TrackerHit3D* getTrackerHit() { return &_trackerHit; };
            
    int getSide() { return _side; }
    unsigned getModule() { return _module; }
    unsigned getSensor() { return _sensor; }
    
    void setSide( int side ){ _side = side; calculateSector();}
    void setLayer( unsigned layer ){ _layer = layer; calculateSector();}
    void setModule( unsigned module ){ _module = module; calculateSector();}
    void setSensor( unsigned sensor ){ _layer = sensor; calculateSector();}
          
    virtual const ISectorSystem* getSectorSystem() const { return _sectorSystemFTD; };
    
  protected:
    
    edm4hep::TrackerHit3D _trackerHit;
            
    int _side;
    unsigned _layer;
    unsigned _module;
    unsigned _sensor;
    
    const SectorSystemFTD* _sectorSystemFTD;
    
    /** Calculates and sets the sector number
     */
    void calculateSector(){ _sector = _sectorSystemFTD->getSector( _side, _layer , _module , _sensor ); }
  };
}
#endif

