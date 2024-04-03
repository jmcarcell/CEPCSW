#ifndef MINIVECTOR_H
#define MINIVECTOR_H 1

#include <iostream>
#include <sstream>
#include <string>
#if __has_include("edm4hep/TrackerHit3D.h")
#include "edm4hep/TrackerHit3D.h"
#else
#include "edm4hep/TrackerHit.h"
namespace edm4hep {
  using TrackerHit3D = edm4hep::TrackerHit;
} // namespace edm4hep
#endif
//#include "UTIL/LCTrackerConf.h"
#include "KiTrack/IHit.h"

#include "ILDImpl/SectorSystemVXD.h"

#include <vector>
#include <math.h>

typedef std::vector<edm4hep::TrackerHit3D*> TrackerHitVec;

namespace KiTrackMarlin{
  class MiniVector : public IHit{
  public:
    TrackerHitVec HitVec ;
    
    // Class constructor
    MiniVector(edm4hep::TrackerHit3D * outer, edm4hep::TrackerHit3D * inner);
    
    MiniVector(TrackerHitVec hitPair);
    
    ~MiniVector();
    
    // returns the TrackerHitVec 
    TrackerHitVec getTrackerHitVec() ;
    
    // Gives the layer of the inner hit
    //int getLayer() ;
    
    // Gives the azimuth angle of the mini-vector
    double getPhi() ;
    
    // Gives the polar angle of the mini-vector
    double getTheta() ;
    
    // Gives the 2-D angle between two minivectors
    //double get2DAngle(MiniVector MinVec1, MiniVector MinVec2) ;
    
    // Gives the 3-D angle between two minivectors
    double get3DAngleMV(MiniVector *MinVec2) ;
    
    double * getXYZ() ;
    
    // Gives the position of the mini-vector
    double * getPosition() ;

    virtual const ISectorSystem* getSectorSystem() const { return _sectorSystemVXD; };
      

  protected:

    const SectorSystemVXD* _sectorSystemVXD;
    
  };

}




#endif
