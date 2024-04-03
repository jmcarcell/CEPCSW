#ifndef ILDVTrackHIT_H
#define ILDVTrackHIT_H

/** ILDVMeasLayer:  Virtual hit class used by ILD[X]Hit Classes, which should provide coordinate vector as defined by the MeasLayer
 *
 * @author S.Aplin DESY
 */


#include "kaltest/TVTrackHit.h"

#include "ILDVMeasLayer.h"

#if __has_include("edm4hep/TrackerHit3D.h")
#include "edm4hep/TrackerHit3D.h"
#else
#include "edm4hep/TrackerHit.h"
namespace edm4hep {
  using TrackerHit3D = edm4hep::TrackerHit;
} // namespace edm4hep
#endif

class ILDVTrackHit : public TVTrackHit {
  
public:
  
   /** Constructor Taking coordinates and associated measurement layer, with bfield and number of measurement dimentions*/
  ILDVTrackHit(const TVMeasLayer &ms, Double_t *x, Double_t *dx, 
               Double_t bfield , Int_t dim, edm4hep::TrackerHit3D trkhit) 
  : TVTrackHit(ms, x, dx, bfield, dim), _trkhit(trkhit)
  { /* no op */ }
  
  edm4hep::TrackerHit3D getLCIOTrackerHit() const { return _trkhit; }
  
private:
  
  edm4hep::TrackerHit3D _trkhit;
  
};
#endif
