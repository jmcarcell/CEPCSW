#ifndef TrackerHitHelper_H
#define TrackerHitHelper_H

#if __has_include("edm4hep/TrackerHit3D.h")
#include "edm4hep/TrackerHit3D.h"
#else
#include "edm4hep/TrackerHit.h"
namespace edm4hep {
  using TrackerHit3D = edm4hep::TrackerHit;
} // namespace edm4hep
#endif
#include "edm4hep/SimTrackerHit.h"
#include "edm4hep/MCRecoTrackerAssociationCollection.h"
#include "DDSegmentation/Segmentation.h"
#include "DetSegmentation/GridDriftChamber.h"
#include <array>

//namespace dd4hep {
//         class Detector;
//         namespace DDSegmentation{
//             class GridDriftChamber;
//         }
//}

namespace CEPC{
  std::array<float, 6> GetCovMatrix(edm4hep::TrackerHit3D& hit, bool useSpacePointerBuilderMethod = false);
  float                GetResolutionRPhi(edm4hep::TrackerHit3D& hit);
  float                GetResolutionZ(edm4hep::TrackerHit3D& hit);
  std::array<float, 6> ConvertToCovXYZ(float dU, float thetaU, float phiU, float dV, float thetaV, float phiV, bool useSpacePointBuilderMethod = false);
  const edm4hep::SimTrackerHit getAssoClosestSimTrackerHit(
          const edm4hep::MCRecoTrackerAssociationCollection* assoHits,
          const edm4hep::TrackerHit3D trackerHit,
          const dd4hep::DDSegmentation::GridDriftChamber* segmentation,
          int docaMehtod);
}

#endif
