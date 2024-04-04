#ifndef ITrackFitterTool_h
#define ITrackFitterTool_h

/*
 * Description:
 *   ITrackFitterTool is used to fit a track candidate to obtain track parameter
 *
 * The interface:
 *   * Fit: peform on tracker hits according prepared geometry
 *
 * Author: FU Chengdong <fucd@ihep.ac.cn>
 */

#include "GaudiKernel/IAlgTool.h"
#include "edm4hep/TrackState.h"
#include <vector>

#if __has_include("edm4hep/TrackerHit3D.h")
#include "edm4hep/TrackerHit3D.h"
#else
#include "edm4hep/TrackerHit.h"
namespace edm4hep {
  using TrackerHit3D = edm4hep::TrackerHit;
} // namespace edm4hep
#endif



namespace edm4hep{
  class MutableTrack;
  class TrackerHit;
}

class ITrackFitterTool: virtual public IAlgTool {
 public:

  DeclareInterfaceID(ITrackFitterTool, 0, 1);
  virtual ~ITrackFitterTool() {}

  virtual int Fit(edm4hep::MutableTrack track, std::vector<edm4hep::TrackerHit3D>& trackHits,
                  const decltype(edm4hep::TrackState::covMatrix)& covMatrix, double maxChi2perHit, bool backward) = 0;
  virtual int Fit(edm4hep::MutableTrack track, std::vector<edm4hep::TrackerHit3D>& trackHits,
                  edm4hep::TrackState trackState, double maxChi2perHit, bool backward) = 0;
  virtual std::vector<std::pair<edm4hep::TrackerHit3D, double> >& GetHitsInFit() = 0;
  virtual std::vector<std::pair<edm4hep::TrackerHit3D, double> >& GetOutliers() = 0;
  virtual void Clear() = 0;
};

#endif
